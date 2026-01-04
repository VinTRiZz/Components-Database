#include "sqlitetable.hpp"

#include "sqliteexecutor.hpp"

#include <Components/Logger/Logger.h>

namespace Database {

SQLiteTable::SQLiteTable(SQLiteDatabase &db) :
    m_executor{std::make_shared<SQLiteExecutor>(db)}
{

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

void SQLiteTable::setTable(const std::string &tableName)
{
    m_name = tableName;
    initColumns();
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
                (cellDataIsNull(col.defaultValue) ? std::string("DEFAULT ") + cellDataToString(col.defaultValue) : "") +
                ","
        ;
        if (!col.referedColumn.empty()) {
            foreignKeys +=
                    " FOREIGN KEY (" + col.name +
                    ") REFERENCES " + col.referedColumn +
                    (col.referenceDeleteAction.empty() ? "" : std::string(" ON DELETE ") + col.referenceDeleteAction) +
                    (col.referenceUpdateAction.empty() ? "" : std::string(" ON UPDATE ") + col.referenceUpdateAction) +
                    ",";
        }
    }
    query.pop_back();
    if (!foreignKeys.empty()) {
        query += ", " + foreignKeys;
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

void SQLiteTable::initColumns()
{
    auto tableInfo = m_executor->exec("PRAGMA table_info(\"" + m_name + "\")");
    if (!tableInfo.has_value()) {
        LOG_WARNING("Failed to configure table", m_name);
        return;
    }

    m_columns.clear();
    for (auto& row : tableInfo.value()) {
        ColumnInfo col;
        int currentColNo = 1;
        col.name = std::get<DBCellString>(row[currentColNo++]).value();
        col.type = columnTypeFromText(std::get<DBCellString>(row[currentColNo++]).value());
        col.canBeNull = (std::get<DBCellInteger>(row[currentColNo++]).value() == 0);
        col.defaultValue = row[currentColNo++];
        col.isPrimaryKey = (std::get<DBCellInteger>(row[currentColNo++]).value() != 0);
        m_columns.push_back(col);
    }


//    // 2. Получаем информацию о внешних ключах
    tableInfo = m_executor->exec("PRAGMA foreign_key_list(" + m_name + ")");
    if (!tableInfo.has_value()) {
        return;
    }
    for (auto& row : tableInfo.value()) {
        auto colFrom = std::find_if(m_columns.begin(), m_columns.end(), [targetName = std::get<DBCellString>(row[3]).value()](auto& colInfo){
            return (colInfo.name == targetName);
        });
        colFrom->referedColumn = std::get<DBCellString>(row[2]).value();
        colFrom->referenceUpdateAction = std::get<DBCellString>(row[5]).value();
        colFrom->referenceDeleteAction = std::get<DBCellString>(row[6]).value();
    }
}

}
