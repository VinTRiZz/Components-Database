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

        enum ColumnType : int {
            CT_UNDEFINED = -1,
            CT_INTEGER,
            CT_TEXT,
        };
        ColumnType type;

        bool isPrimaryKey {false};
        std::optional<DBCell> defaultValue;
        bool canBeNull {true};

        std::string referedColumn;
        std::string referenceDeleteAction {"CASCADE"};
    };

    SQLiteTable(const std::string& tableName, SQLiteDatabase& db);

    std::string getName() const;

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
    std::string m_name;
    std::list<ColumnInfo> m_columns;

    std::string columnTypeToText(ColumnInfo::ColumnType ct) const;
    ColumnInfo::ColumnType columnTypeFromText(const std::string& ct) const;
};

}
