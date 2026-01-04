#pragma once

#include "databasecommon.hpp"

#include <list>
#include <map>

namespace Database {

class SQLiteDatabase;
class SQLiteExecutor;

class SQLiteTable
{
public:
    struct ColumnInfo
    {
        std::string name;
        ColumnType type;

        bool isPrimaryKey {false};
        DBCell defaultValue;
        bool canBeNull {true};

        std::string referedColumn;
        std::string referenceDeleteAction {"CASCADE"};
    };

    SQLiteTable(SQLiteDatabase& db);

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    void setTable(const std::string& tableName);
    std::string getName() const;
    bool isTableExist() const;
    std::string getLastError() const;

    bool create(const std::list<ColumnInfo>& columns);
    bool addColumn(const ColumnInfo& columnConfig);
    std::list<ColumnInfo> getColumns() const;
    bool removeColumn(const std::string& columnName);
    bool drop();

    bool addRow(DBRow&& rowData);
    bool addRow(std::map<std::string, DBCell>&& rowNamedData);
    bool updateRow(std::map<std::string, DBCell>&& rowNamedData, const std::string& whereCondition = {});
    bool removeRow(const std::string& whereCondition = {});
    std::vector<DBRow> getRow(const std::string& whereCondition = {}, const std::string& orderCondition = {}) const;

private:
    std::shared_ptr<SQLiteExecutor> m_executor;

    std::string m_name;
    std::list<ColumnInfo> m_columns;

    void initColumns();
    std::optional<DBCell> parseDefaultValue(const char* sqlite_default, ColumnType col_type);
};

}
