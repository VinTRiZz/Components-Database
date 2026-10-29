#pragma once

#include <unordered_set>

#include <Components/ExtraClasses/Containers/Handler.h>

#include "tableitembase.hpp"

namespace Database::Orm {

using ItemContainer = std::unordered_set<ItemHandler>;
using RowsHandler = ExtraClasses::Handler<const ItemContainer>;

class TableItemMaster;
using TableItemMasterHdl = ExtraClasses::Handler<TableItemMaster>;

/**
 * @brief The TableItemMaster class Handle for a table, providing objects, according to Active Record pattern
 */
class TableItemMaster : public ExtraClasses::ErrorUserBase<ExtraClasses::ErrorObject>
{
public:
    TableItemMaster(const RecordManager& recordManager);
    ~TableItemMaster();

    TableItemMasterHdl createHandler() const;
    RecordManager& getRecordManager() const;

    void setTable(const std::string& tableName);
    std::optional<std::string> getTable() const;

    using selector_t = std::function<bool(const ItemHandler&)>;

    virtual bool createTable();
    bool isTableExist() const;
    bool loadRecords(); // TODO: Work with dynamic load, if even needed
    virtual void setDatabaseFilter(const std::string& recordsFilter);
    virtual void setFilter(selector_t&& selector);


    // CRUD of rows
    ItemHandler createTemporaryItem();              // Creates temporary item to save manually, will have invalid ID
    bool        insertItem(ItemHandler hdl);        // Inserts row into table (saves created temporary item)
    bool        updateItem(const ItemHandler &hdl); // Updates row in table
    bool        removeItem(ItemHandler hdl);        // Removes item with id. Also invalidates handlers

    // Search not only in visible, but in temporary items too. Ignore filters flag is mostly for item saving
    ItemHandler findItem(selector_t&& selector, bool ignoreFilters = false) const;

    // Utility getters
    ItemHandler getItem(const DBCell& id) const;
    ItemHandler getItemByRow(int itemRow) const;
    int         getItemRow(const ItemHandler& hdl) const;
    RowsHandler getAllItems() const;

    // Updating
    void reloadRecords();
    void updateFilter();

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void processItemAdd(const ItemHandler& hdl);
    void processItemUpdate(const ItemHandler& hdl);
    void processItemRemove(const ItemHandler& hdl);

    void resetRows();

protected:
    virtual ItemHandler createItem() = 0;
};

}