#include "sqlitetable.hpp"

#include "sqliteexecutor.hpp"
#include "sqlitedatabase.h"

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
    if (columns.empty()) {
        LOG_ERROR("Can not create table with no columns");
        return false;
    }

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
    // ALTER TABLE table_name ADD COLUMN column_name column_type;

    if (columnConfig.referedColumn.empty()) {
        auto columnQuery =
                columnConfig.name + " " +
                columnTypeToText(columnConfig.type) + " " +
                (columnConfig.isPrimaryKey ? "PRIMARY KEY AUTOINCREMENT " : "") +
                (columnConfig.canBeNull ? "" : "NOT NULL ") +
                (cellDataIsNull(columnConfig.defaultValue) ? std::string("DEFAULT ") + cellDataToString(columnConfig.defaultValue) : "");

        auto res = m_executor->exec(std::string("ALTER TABLE ") + m_name + " ADD COLUMN " + columnQuery).has_value();
        if (res) {
            m_columns.push_back(columnConfig);

            // Во избежание ошибок юзера, делаем чистку
            auto& lastCol = m_columns.back();
            lastCol.referenceDeleteAction = {};
            lastCol.referenceUpdateAction = {};
        }
        return res;
    }

    std::string colsQuery;
    for (auto& col : m_columns) {
        colsQuery += col.name + ",";
    }
    colsQuery.pop_back();

    // Запомним конфиг новой таблицы
    m_columns.push_back(columnConfig);
    auto colsCopy = m_columns;
    auto selfName = m_name;
    auto tempTableName = std::string("new_") + selfName;

    // Создаём временную таблицу
    setTable(tempTableName);
    auto currentOperationRes = create(colsCopy);
    if (!currentOperationRes) {
        return false;
    }

    currentOperationRes = m_executor->exec(std::string("INSERT INTO ") + tempTableName + " SELECT " + colsQuery + ", NULL FROM " + selfName).has_value();
    if (!currentOperationRes) {
        return false;
    }

    setTable(selfName);
    currentOperationRes = drop();
    if (!currentOperationRes) {
        return false;
    }

    m_columns.push_back(columnConfig);
    currentOperationRes = m_executor->exec("ALTER TABLE " + tempTableName + " RENAME TO " + selfName).has_value();
    if (!currentOperationRes) {
        return false;
    }

    setTable(selfName);
    return true;
}

std::list<SQLiteTable::ColumnInfo> SQLiteTable::getColumns() const
{
    return m_columns;
}

bool SQLiteTable::removeColumn(const std::string &columnName)
{
    /*
    CREATE TABLE new_table_name AS SELECT col1, col3 FROM old_table_name;

    INSERT INTO new_table_name SELECT col1, col3 FROM old_table_name;

    DROP TABLE old_table_name;

    ALTER TABLE new_table_name RENAME TO old_table_name;
    */

    std::string colsQuery;
    for (auto& col : m_columns) {
        if (col.name == columnName) {
            continue;
        }
        colsQuery += col.name + ",";
    }
    colsQuery.pop_back();
    auto colsCopy = m_columns;

    m_columns.erase(std::find_if(m_columns.begin(), m_columns.end(), [columnName](auto& colInfo){
        return (colInfo.name == columnName);
    }));
    auto currentOperationRes = m_executor->exec(std::string("CREATE TABLE new_") + m_name + " AS SELECT " + colsQuery + " FROM " + m_name).has_value();
    if (!currentOperationRes) {
        m_columns = colsCopy;
        return false;
    }

    currentOperationRes = m_executor->exec(std::string("INSERT INTO new_") + m_name + " SELECT " + colsQuery + " FROM " + m_name).has_value();
    if (!currentOperationRes) {
        m_columns = colsCopy;
        return false;
    }

    auto selfName = m_name;
    currentOperationRes = drop();
    if (!currentOperationRes) {
        m_name = selfName;
        m_columns = colsCopy;
        return false;
    }

    currentOperationRes = m_executor->exec("ALTER TABLE new_" + selfName + " RENAME TO " + selfName).has_value();
    if (!currentOperationRes) {
        m_name = selfName;
        m_columns = colsCopy;
        return false;
    }

    setTable(selfName);
    return true;
}

bool SQLiteTable::drop()
{
    auto res = m_executor->exec(std::string("DROP TABLE ") + m_name).has_value();
    if (res) {
        m_columns.clear();
        m_name = {};
    }
    return res;
}

bool SQLiteTable::addRow(DBRow &&rowData)
{
    std::string query = "INSERT INTO ";

    std::string colsQuery;
    for (auto& col : m_columns) {
        colsQuery += col.name + ",";
    }
    colsQuery.pop_back();

    query += m_name + " (" + colsQuery + ") VALUES (";

    for (auto& v : rowData) {
        query += cellDataToString(v) + ",";
    }
    query.pop_back();
    query += ")";

    return m_executor->exec(query).has_value();
}

bool SQLiteTable::addRow(std::map<std::string, DBCell> &&rowNamedData)
{
    std::string colsQuery;
    std::string valuesQuery;
    for (auto& [colName, colValue] : rowNamedData) {
        colsQuery += colName + ",";
        valuesQuery += cellDataToString(colValue) + ",";
    }
    colsQuery.pop_back();
    valuesQuery.pop_back();

    return m_executor->exec(std::string("INSERT INTO ") + m_name + " (" + colsQuery + ") VALUES (" + valuesQuery + ")").has_value();
}

bool SQLiteTable::updateRow(std::map<std::string, DBCell> &&rowNamedData, const std::string &whereCondition)
{
    // UPDATE tasks SET name='Test update' WHERE id=1

    std::string query = "UPDATE " + m_name + " SET ";

    std::string dataQuery;
    for (auto& [colName, colValue] : rowNamedData) {
        query += colName + "=" + cellDataToString(colValue) + ",";
    }
    query.pop_back();
    query += (whereCondition.empty() ? "" : std::string(" WHERE ") + whereCondition);

    return m_executor->exec(query).has_value();
}

bool SQLiteTable::removeRow(const std::string &whereCondition)
{
    // DELETE FROM tasks WHERE name='Test string'

    std::string query = "DELETE FROM " + m_name + (whereCondition.empty() ? "" : std::string(" WHERE ") + whereCondition);
    return m_executor->exec(query).has_value();
}

std::vector<DBRow> SQLiteTable::getRows(const std::vector<std::string>& cols, const std::string &whereCondition, const std::string &orderCondition) const
{
    // SELECT id FROM tasks WHERE name='Test string'

    std::string query;
    if (cols.empty()) {
        query = std::string("SELECT * FROM ") + m_name;
    } else {
        query = "SELECT ";
        for (auto& col : cols) {
            query += col + ",";
        }
        query.pop_back();
        query += " FROM " + m_name;
    }
    return m_executor->exec(query +
                            (whereCondition.empty() ? "" : std::string(" WHERE ") + whereCondition) +
                            (orderCondition.empty() ? "" : std::string(" ORDER BY ") + orderCondition), true).value();
}

void SQLiteTable::initColumns()
{
    auto tableInfo = m_executor->exec("PRAGMA table_info(\"" + m_name + "\")", true);
    if (!tableInfo.has_value()) {
        LOG_WARNING("Failed to configure table", m_name, "reason: no table data found");
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
    tableInfo = m_executor->exec("PRAGMA foreign_key_list(" + m_name + ")", true);
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
