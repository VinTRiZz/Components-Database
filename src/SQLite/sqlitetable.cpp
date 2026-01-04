#include "sqlitetable.hpp"

#include "sqliteexecutor.hpp"

namespace Database {

SQLiteTable::SQLiteTable(const std::string &tableName, SQLiteDatabase &db) :
    m_executor{std::make_shared<SQLiteExecutor>(db)},
    m_name{tableName}
{
    //    if (!m_db.exec(
    //        std::string("PRAGMA table_info(") + m_targetTableName + ")",
    //        [this](int argc, char **argv, char **azColName) {

    //        std::string tableColumnName = "";
    //        int colType = 0;

    //        for (int i = 0; i < argc; i++) {
    //            std::string colName = azColName[i];

    //            if (argv[i]) {
    //                if (colName == "name") {
    //                    tableColumnName = argv[i];
    //                } else if (colName == "type") {

    //                    std::string typeString = argv[i];
    //                    if (typeString == "INTEGER") {
    //                        colType = 1;
    //                    } else if (typeString == "TEXT") {
    //                        colType = 2;
    //                    }
    //                }
    //            }
    //        }
    //        m_columnTypeBufferMap[tableColumnName] = colType;
    //    }, m_lastErrorMessage)) {
    //        return false;
    //    }
}

bool SQLiteTable::beginTransaction()
{
    return m_executor->beginTransaction();
}

bool SQLiteTable::commitTransaction()
{
    return m_executor->commitTransaction();
}

bool SQLiteTable::rollbackTransaction()
{
    return m_executor->rollbackTransaction();
}

std::string SQLiteTable::getName() const
{
    return m_name;
}

bool SQLiteTable::isTableExist() const
{
    auto res = m_executor->exec(std::string("PRAGMA table_info(") + m_name + ")");
    return (res.has_value() && (res.value().size() > 0));
}

std::string SQLiteTable::getLastError() const
{
    return m_executor->getLastError();
}

bool SQLiteTable::create(const std::list<ColumnInfo> &columns)
{
    std::string query = "CREATE TABLE " + m_name + " (";
    std::string foreignKeys;

    for (auto& col : columns) {
        query +=
                col.name + " " +
                columnTypeToText(col.type) + " " +
                (col.isPrimaryKey ? "PRIMARY KEY AUTOINCREMENT " : "") +
                (col.canBeNull ? "" : "NOT NULL ") +
                (col.defaultValue.has_value() ? std::string("DEFAULT ") + cellDataToString(col.defaultValue.value()) : "") +
                ","
        ;
        if (!col.referedColumn.empty()) {
            foreignKeys +=
                    " FOREIGN KEY (" + col.name +
                    ") REFERENCES " + col.referedColumn +
                    " ON DELETE " + col.referenceDeleteAction +
                    ",";
        }
    }
    query.pop_back();
    if (!foreignKeys.empty()) {
        query += foreignKeys;
        query.pop_back();
    }
    query += ")";
    if (m_executor->exec(query).has_value()) {
        m_columns = columns;
        return true;
    }
    return false;
}

bool SQLiteTable::addColumn(const ColumnInfo &columnConfig)
{
    return false;
}

std::list<SQLiteTable::ColumnInfo> SQLiteTable::getColumns() const
{
    return m_columns;
}

bool SQLiteTable::removeColumn(const std::string &columnName)
{
    return false;
}

bool SQLiteTable::drop()
{
    return m_executor->exec(std::string("DROP TABLE ") + m_name).has_value();
}

bool SQLiteTable::addRow(DBRow &&rowData)
{
//    m_lastQuery = "INSERT INTO ";
//    m_targetTableName = tableName;

//    m_lastQuery += tableName + " VALUES (" + getCellValueString(values[0]);
//    for (std::size_t i = 1; i < values.size(); i++) {

//        m_lastQuery += ", " + getCellValueString(values[i]);
//    }
//    m_lastQuery += ")";

//    m_querryRows.clear();
//    if (!defaultExec()) {
//        return false;
//    }
//    m_currentValueIt = m_querryRows.begin();
//    return true;
}

bool SQLiteTable::addRow(std::map<std::string, DBCell> &&rowNamedData)
{
    // INSERT INTO tasks VALUES (1, 'test string')

//    m_lastQuery = "INSERT INTO ";
//    m_targetTableName = tableName;

//    m_lastQuery += tableName + " VALUES (" + getCellValueString(values[0]);
//    for (std::size_t i = 1; i < values.size(); i++) {

//        m_lastQuery += ", " + getCellValueString(values[i]);
//    }
//    m_lastQuery += ")";

//    m_querryRows.clear();
//    if (!defaultExec()) {
//        return false;
//    }
//    m_currentValueIt = m_querryRows.begin();
//    return true;
}

bool SQLiteTable::updateRow(std::map<std::string, DBCell> &&rowNamedData, const std::string &whereCondition)
{
    // UPDATE tasks SET name='Test update' WHERE id=1

//    m_lastQuery = "UPDATE ";
//    m_targetTableName = tableName;
//    m_queryColumns = columns;

//    m_lastQuery += tableName + " SET (" + columns[0] + "=" + getCellValueString(values[0]);
//    int pos = 0;
//    for (auto& colStr : columns) {
//        m_lastQuery += ", " + colStr + "=" + getCellValueString(values[pos++]);
//    }
//    m_lastQuery += ") " + criteriaStr;

//    m_querryRows.clear();
//    return defaultExec();
}

bool SQLiteTable::removeRow(const std::string &whereCondition)
{
    // DELETE FROM tasks WHERE name='Test string'

//    m_lastQuery = "DELETE FROM ";
//    m_targetTableName = tableName;

//    m_lastQuery += tableName + " " + criteriaStr;

//    m_querryRows.clear();
//    return defaultExec();
}

std::vector<DBRow> SQLiteTable::getRow(const std::string &whereCondition, const std::string &orderCondition) const
{
    // SELECT id FROM tasks WHERE name='Test string'

//    m_lastQuery = "SELECT ";
//    m_targetTableName = tableName;
//    m_queryColumns = columns;

//    m_lastQuery += columns[0];
//    for (std::size_t i = 1; i < columns.size(); i++) {
//        m_lastQuery += ", " + columns[i];
//    }
//    m_lastQuery += " FROM ";
//    m_lastQuery += tableName;

//    if (!criteriaStr.empty()) {
//        m_lastQuery += " WHERE " + criteriaStr;
//    }

//    m_querryRows.clear();
//    if (!defaultExec()) {
//        return false;
//    }
//    m_currentValueIt = m_querryRows.begin();
    //    return true;
}

}
