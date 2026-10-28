#include "tableitemmaster.hpp"

#include <Components/Logger/Logger.h>

#include <unordered_set>

namespace Database::Orm {

struct TableItemMaster::Impl
{
    // System
    TableItemMasterHdl          m_masterHandler;
    RecordManager               m_db;
    std::optional<std::string>  m_tableName {};
    std::string                 m_recordsFilter;

    // Item handling
    std::unordered_set<ItemHandler> m_temporaryItems;
    ItemContainer   m_tableRows;
    ItemContainer   m_filteredRows;
    selector_t      m_filter;
    RowsHandler     m_dummyRowHandler { &m_filteredRows }; // For getters
};

TableItemMaster::TableItemMaster(const RecordManager &recordManager) :
    d { new Impl{.m_db = recordManager} }
{
    d->m_masterHandler = this;
}

TableItemMaster::~TableItemMaster()
{
    d->m_masterHandler.invalidate();
    d->m_dummyRowHandler.invalidate();
}

TableItemMasterHdl TableItemMaster::createHandler() const
{
    return d->m_masterHandler;
}

RecordManager &TableItemMaster::getRecordManager() const
{
    return d->m_db;
}

void TableItemMaster::setTable(const std::string &tableName)
{
    resetRows();
    d->m_tableName = tableName;
}

std::optional<std::string> TableItemMaster::getTable() const
{
    return d->m_tableName;
}

bool TableItemMaster::createTable()
{
    // Must be implemented in inheritors
    return false;
}

bool TableItemMaster::isTableExist() const
{
    if (!d->m_tableName.has_value()) { return false; }
    auto pCon = d->m_db.getConnection();
    auto values = pCon->executeQuery(std::string("PRAGMA table_info(\"") + d->m_tableName.value() + "\")");
    return values.has_value();
}

bool TableItemMaster::loadRecords()
{
    resetRows();
    if (!d->m_tableName.has_value()) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseInvalidTable);
        m_error.setDetailText("Table name not set");
        COMPLOG_ERROR("TableItemMaster (", getTable(), ") : failed to load records:", std::string(getError().what()));
        return false;
    }

    auto records = d->m_db.getAllRecords<TableItemBase>();
    for (auto& rec : records) {
        auto itemHdl = createItem();
        itemHdl->setRecord(rec.getRecord());
        d->m_tableRows.insert(itemHdl);
    }

    updateFilter();

    COMPLOG_INFO("[", getTable(), "] loaded objects:", d->m_tableRows.size(), "filtered to:", d->m_filteredRows.size(), ")");
    return true;
}

void TableItemMaster::setDatabaseFilter(const std::string &recordsFilter)
{
    d->m_recordsFilter = recordsFilter;
}

void TableItemMaster::setFilter(selector_t &&selector)
{
    d->m_filter = selector;
    updateFilter();
}

ItemHandler TableItemMaster::createTemporaryItem()
{
    auto itemHdl = createItem();
    d->m_temporaryItems.insert(itemHdl);
    return itemHdl;
}

bool TableItemMaster::insertItem(ItemHandler hdl)
{
    if (!hdl || std::holds_alternative<std::monostate>(hdl->getId())) {
        m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
        m_error.setDetailText("invalid handle of item to save");
        return false;
    }

    auto rowData = hdl->getRecord();
    if (rowData.empty()) {
        COMPLOG_ERROR("TableItemMaster (", getTable(), ") : failed convertion to values");
        return false;
    }

    auto insertedId = d->m_db.addRecord(*hdl);
    if (!insertedId.has_value()) {
        return false;
    }

    hdl->setId(insertedId.value());
    d->m_temporaryItems.erase(hdl);
    d->m_tableRows.insert(hdl);
    if (d->m_filter) {
        if (d->m_filter(hdl)) {
            processItemAdd(hdl);
        }
    } else {
        processItemAdd(hdl);
    }
    return true;
}

bool TableItemMaster::updateItem(const ItemHandler& hdl)
{
    if (!hdl) {
        m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
        m_error.setDetailText("invalid handle of item to update");
        return false;
    }

    auto rowData = hdl->getRecord();
    if (rowData.empty()) {
        COMPLOG_ERROR("TableItemMaster (", getTable(), ") : failed convertion to values:", hdl->getError().what());
        return false;
    }

    rowData.erase("id");
    if (!d->m_db.updateRecord(*hdl)) {
        return false;
    }
    processItemUpdate(hdl);
    updateFilter();
    return true;
}

bool TableItemMaster::removeItem(ItemHandler hdl)
{
    if (!hdl) {
        m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
        m_error.setDetailText("invalid handle of item to remove");
        return false;
    }

    if (!std::holds_alternative<std::monostate>(hdl->getId())) { // Item is temporary or have been removed
        auto pItem = hdl.get();
        d->m_temporaryItems.erase(hdl);
        hdl.invalidate();
        delete pItem;
        return true;
    }

    if (!d->m_db.removeRecord(*hdl)) {
        return false;
    }
    auto pItem = hdl.get();
    processItemRemove(hdl);
    d->m_tableRows.erase(hdl);
    hdl.invalidate();
    delete pItem;
    return true;
}

ItemHandler TableItemMaster::getItem(const DBCell &id) const
{
    return findItem([&](const auto& hdl) -> bool {
        return (id == hdl->getId());
    });
}

ItemHandler TableItemMaster::getItemByRow(int itemRow) const
{
    if (itemRow < 0 || itemRow >= d->m_filteredRows.size()) { return {}; }
    auto itemIt = d->m_filteredRows.begin();
    std::advance(itemIt, itemRow);
    return *itemIt;
}

int TableItemMaster::getItemRow(const ItemHandler &hdl) const
{
    if (!hdl || std::holds_alternative<std::monostate>(hdl->getId())) {
        return -1;
    }
    auto targetIt = d->m_filteredRows.find(hdl);
    if (d->m_filteredRows.end() == targetIt) {
        return -1;
    }
    return std::distance(d->m_filteredRows.begin(), targetIt);
}

ItemHandler TableItemMaster::findItem(std::function<bool (const ItemHandler &)> &&selector, bool ignoreFilters) const
{
    for (auto& itemHdl : d->m_temporaryItems) {
        if (selector(itemHdl)) {
            return itemHdl;
        }
    }

    if (ignoreFilters) {
        for (auto& itemHdl : d->m_tableRows) {
            if (selector(itemHdl)) {
                return itemHdl;
            }
        }
    } else {
        for (auto& itemHdl : d->m_filteredRows) {
            if (selector(itemHdl)) {
                return itemHdl;
            }
        }
    }
    return {};
}

RowsHandler TableItemMaster::getAllItems() const
{
    return d->m_dummyRowHandler;
}

void TableItemMaster::reloadRecords()
{
    loadRecords();
}

void TableItemMaster::resetRows()
{
    for (auto pItemHdl : d->m_temporaryItems) {
        auto pItem = pItemHdl.get();
        pItemHdl.invalidate();
        delete pItem;
    }
    d->m_temporaryItems.clear();

    d->m_filteredRows.clear();
    for (auto pItemHdl : d->m_tableRows) {
        auto pItem = pItemHdl.get();
        pItemHdl.invalidate();
        delete pItem;
    }
    d->m_tableRows.clear();
}

void TableItemMaster::updateFilter()
{
    if (!d->m_filter || d->m_tableRows.empty()) {
        d->m_filteredRows = d->m_tableRows;
        return;
    }

    // Reserve delta
    std::size_t buckets = d->m_filteredRows.bucket_count();
    float maxLoad = d->m_filteredRows.max_load_factor();
    auto currentCapacity = static_cast<std::size_t>(buckets * maxLoad);
    if (currentCapacity < d->m_tableRows.size()) {
        d->m_filteredRows.reserve(d->m_tableRows.size() - currentCapacity);
    }

    // Process filters
    for (const auto& hdl : d->m_tableRows) {
        auto isCorrect = d->m_filter(hdl);
        if (!isCorrect) {
            auto removedHdlIt = d->m_filteredRows.find(hdl);
            if (d->m_filteredRows.end() != removedHdlIt) {
                processItemRemove(hdl);
            }
            continue;
        }
        auto removedHdlIt = d->m_filteredRows.find(hdl);
        if (d->m_filteredRows.end() == removedHdlIt) {
            processItemAdd(hdl);
        }
    }
}

void TableItemMaster::processItemAdd(const ItemHandler &hdl)
{
    d->m_filteredRows.insert(hdl);
}

void TableItemMaster::processItemUpdate([[maybe_unused]] const ItemHandler &hdl)
{
    // Do nothing (now not implemented)
}

void TableItemMaster::processItemRemove(const ItemHandler &hdl)
{
    d->m_filteredRows.erase(hdl);
}

}
