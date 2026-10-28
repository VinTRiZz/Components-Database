#include "tableitembase.hpp"

#include "tableitemmaster.hpp"

namespace Database::Orm {

TableItemBase::TableItemBase(const TableItemMasterHdl& pTableMaster) :
    RecordBaseS(),
    m_tableMaster {pTableMaster}
{

}

std::optional<std::string> TableItemBase::getTableName() const {
    return m_tableMaster.isValid() ? std::optional<std::string>(m_tableMaster->getTable()) : std::nullopt;
}

void TableItemBase::setId(const DBCell &id) {
    setField("id", id);
}

DBCell TableItemBase::getId() const {
    return getField("id");
}

bool TableItemBase::saveItem() {
    m_rowData.commit();

    bool isSucceed {false};
    if (std::holds_alternative<std::monostate>(getId())) {
        auto hdl = m_tableMaster->findItem([this](const auto& hdl) -> bool {
            return (hdl == this);
        });
        isSucceed = m_tableMaster->insertItem(hdl);
    } else {
        const auto id = getId();
        auto hdl = m_tableMaster->findItem([id](const auto& hdl) -> bool {
            return (id == hdl->getId());
        }, true);
        isSucceed = m_tableMaster->updateItem(hdl);
    }
    return isSucceed;
}

void TableItemBase::restoreSaved() {
    m_rowData.revert();
}

void TableItemBase::removeItem() {
    auto hdl = m_tableMaster->findItem([this](const auto& hdl) -> bool {
        return (this == hdl.get());
    });
    m_tableMaster->removeItem(hdl);
}

void TableItemBase::setRecord(const DBRowNamed &values) {
    m_rowData = values;
    m_rowData.commit();
}

DBRowNamed TableItemBase::getRecord() const {
    return m_rowData;
}

bool TableItemBase::operator==(const TableItemBase &_oItem) const {
    return  (m_tableMaster == _oItem.m_tableMaster) &&
           (m_rowData.value() == _oItem.m_rowData.value());
}

bool TableItemBase::operator!=(const TableItemBase &_oItem) const {
    return !(*this == _oItem);
}

DBCell TableItemBase::getField(const std::string &fieldName) const {
    auto targetField = m_rowData->find(fieldName);
    if (targetField == m_rowData->end()) {
        return {};
    }
    return targetField->second;
}

void TableItemBase::setField(const std::string &fieldName, const DBCell &fieldValue) {
    (*m_rowData)[fieldName] = fieldValue;
}

}
