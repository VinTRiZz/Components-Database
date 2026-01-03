#pragma once

#include "databasecommon.hpp"

#include "sqliteexecutor.hpp"

#include <list>
#include <map>

namespace Database {

class SQLiteTable : public SQLiteExecutor
{
public:
    struct ColumnInfo
    {
        std::string name;
        std::string type;
        std::string ref;
        std::string rules;
    };
    using SQLiteExecutor::SQLiteExecutor;

    bool create(const std::list<ColumnInfo>& columns);
    bool addColumn(const ColumnInfo& columnConfig);
    bool removeColumn(const std::string& columnName);
    bool drop();

    bool addRow(DBRow&& rowData);
    bool addRow(std::map<std::string, DBCell>&& rowNamedData);
    bool updateRow(std::map<std::string, DBCell>&& rowNamedData, const std::string& whereCondition = {});
    bool removeRow(const std::string& whereCondition = {});
    std::vector<DBRow> getRow(const std::string& whereCondition = {}, const std::string& orderCondition = {}) const;

private:
    std::list<ColumnInfo> m_columns;
};

}
