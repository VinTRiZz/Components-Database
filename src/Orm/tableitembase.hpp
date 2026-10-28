#pragma once

#include <Components/ExtraClasses/Containers/Handler.h>
#include <Components/ExtraClasses/Containers/CommitableObject.h>
#include <Components/ExtraClasses/Error.h>

#include <Components/Database/RecordManager.h>

namespace Database::Orm {

class TableItemMaster;
using TableItemMasterHdl = ExtraClasses::Handler<TableItemMaster>;

/**
 * @brief The TableItemBase class Base of a row from database, according to Active Record pattern
 * @note deleted item object = removed row from db
 */
class TableItemBase : public RecordBaseS
{
public:
    explicit TableItemBase(const TableItemMasterHdl &pTableMaster = {});
    std::optional<std::string> getTableName() const;

    /**
     * @brief setId Changes ID of an item
     * @param id
     */
    virtual void setId(const DBCell &id);
    DBCell getId() const;

    /**
     * @brief saveItem Add or update item's associated record in DB
     */
    virtual bool saveItem();

    /**
     * @brief restoreSaved Reverse changes to previous saved / loaded state. Does not reverse ID changes
     * @note In base class does nothing, but in inheried must restore fields
     */
    virtual void restoreSaved();

    /**
     * @brief removeItem Remove item from DB
     */
    virtual void removeItem();

    /**
     * @brief setRecord Read values from record
     * @param values
     */
    void setRecord(const DBRowNamed& values);
    DBRowNamed getRecord() const;

    bool operator==(const TableItemBase& _oItem) const;
    bool operator!=(const TableItemBase& _oItem) const;

private:
    TableItemMasterHdl m_tableMaster {}; // Used to determine if item is valid
    ExtraClasses::CommitableObject<DBRowNamed>  m_rowData;

protected:
    DBCell  getField(const std::string& fieldName) const;
    void    setField(const std::string& fieldName, const DBCell& fieldValue);
};
using ItemHandler = ExtraClasses::Handler<TableItemBase>;

}