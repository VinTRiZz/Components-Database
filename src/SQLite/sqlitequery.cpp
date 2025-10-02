#include "sqlitequery.h"

#include "sqlitedatabase.h"
#include <stdexcept>

#include <Components/Logger/Logger.h>

namespace SQLiteAdapter
{

std::string getCellValueString(const InputCellValue& cellv)
{
    if (!cellv.has_value()) {
        return "null";
    }

    if (cellv->type() == typeid(std::string)) {
        return std::string("\'") + boost::get<std::string>(cellv.value()) + "\'"; // TEXT (string)
    } else if (cellv->type() == typeid(int64_t)) {
        return std::to_string(boost::get<int64_t>(cellv.value()));                // INTEGER (int)
    }
    return {};
}

SQLiteQuery::SQLiteQuery(SQLiteDatabase &db) :
    m_db {db}
{

}

SQLiteQuery::SQLiteQuery(const SQLiteQuery &other) :
    m_db {other.m_db}
{

}

bool SQLiteQuery::exec(const std::string &m_lastQuery)
{
    return defaultExec();
}

bool SQLiteQuery::createTable(const std::string &tableName,
                              const std::vector<std::string> &columns,
                              const std::vector<std::pair<std::string, std::string>> foreignKeys)
{
    m_lastQuery = "CREATE TABLE ";
    m_targetTableName = tableName;
    m_queryColumns = columns;

    m_lastQuery += tableName + " (" + columns[0];
    int pos = 0;
    for (std::size_t i = 1; i < columns.size(); i++) {
        m_lastQuery += ", " + columns[i];
    }

    for (auto& fkPair : foreignKeys) {
        m_lastQuery += ", FOREIGN KEY (" + fkPair.first + ") REFERENCES " + fkPair.second + " ON DELETE CASCADE";
    }

    m_lastQuery += ") ";
    return defaultExec();
}

bool SQLiteQuery::dropTable(const std::string &tableName)
{
    // DROP TABLE tasks

    m_targetTableName = "";
    m_lastQuery = std::string("DROP TABLE ") + tableName;
    return defaultExec();
}

bool SQLiteQuery::select(const std::string &tableName,
                         const std::vector<std::string> &columns,
                         const std::string &criteriaStr)
{
    // SELECT id FROM tasks WHERE name='Test string'

    m_lastQuery = "SELECT ";
    m_targetTableName = tableName;
    m_queryColumns = columns;

    m_lastQuery += columns[0];
    for (std::size_t i = 1; i < columns.size(); i++) {
        m_lastQuery += ", " + columns[i];
    }
    m_lastQuery += " FROM ";
    m_lastQuery += tableName;

    if (!criteriaStr.empty()) {
        m_lastQuery += " WHERE " + criteriaStr;
    }

    m_querryRows.clear();
    if (!defaultExec()) {
        return false;
    }
    m_currentValueIt = m_querryRows.begin();
    return true;
}

bool SQLiteQuery::add(const std::string &tableName,
                      const std::vector<InputCellValue> &values)
{
    // INSERT INTO tasks VALUES (1, 'test string')

    m_lastQuery = "INSERT INTO ";
    m_targetTableName = tableName;

    m_lastQuery += tableName + " VALUES (" + getCellValueString(values[0]);
    for (std::size_t i = 1; i < values.size(); i++) {

        m_lastQuery += ", " + getCellValueString(values[i]);
    }
    m_lastQuery += ")";

    m_querryRows.clear();
    if (!defaultExec()) {
        return false;
    }
    m_currentValueIt = m_querryRows.begin();
    return true;
}

bool SQLiteQuery::update(const std::string &tableName,
                         const std::vector<std::string> &columns,
                         const std::vector<InputCellValue> &values,
                         const std::string &criteriaStr)
{
    // UPDATE tasks SET name='Test update' WHERE id=1

    m_lastQuery = "UPDATE ";
    m_targetTableName = tableName;
    m_queryColumns = columns;

    m_lastQuery += tableName + " SET (" + columns[0] + "=" + getCellValueString(values[0]);
    int pos = 0;
    for (auto& colStr : columns) {
        m_lastQuery += ", " + colStr + "=" + getCellValueString(values[pos++]);
    }
    m_lastQuery += ") " + criteriaStr;

    m_querryRows.clear();
    return defaultExec();
}

bool SQLiteQuery::remove(const std::string &tableName,
                         const std::string &criteriaStr)
{
    // DELETE FROM tasks WHERE name='Test string'

    m_lastQuery = "DELETE FROM ";
    m_targetTableName = tableName;

    m_lastQuery += tableName + " " + criteriaStr;

    m_querryRows.clear();
    return defaultExec();
}

int16_t SQLiteQuery::indexOf(const std::string &columnName)
{
    int16_t indx {-1};
    for (auto& colP : m_queryColumns) {
        if (colP == columnName) {
            break;
        }
        indx++;
    }
    return indx;
}

std::string SQLiteQuery::columnName(int16_t columnIndex)
{
    if (!m_querryRows.size()) {
        return "";
    }

    if (columnIndex < m_queryColumns.size()) {
        return m_queryColumns.at(columnIndex);
    }
    return "";
}

std::size_t SQLiteQuery::rowCount()
{
    return m_querryRows.size();
}

int16_t SQLiteQuery::columnCount()
{
    return m_currentValueIt->size();
}

CellValue SQLiteQuery::value(int16_t index)
{
    if (index < m_currentValueIt->size()) {
        auto resVal = m_currentValueIt->at(index);
        return resVal.has_value() ? resVal.value() : CellValue();
    }
    return {};
}

CellValue SQLiteQuery::value(const std::string &columnName)
{
    auto indx = indexOf(columnName);
    return value(indx);
}

void SQLiteQuery::goNext()
{
    if (m_currentValueIt != m_querryRows.end())
        m_currentValueIt++;
}

std::string SQLiteQuery::lastError() const
{
    return m_lastErrorMessage;
}

std::string SQLiteQuery::lastQuery() const
{
    return m_lastQuery;
}

bool SQLiteQuery::defaultExec()
{
    m_columnTypeBufferMap.clear();

    if (!m_db.exec(
        std::string("PRAGMA table_info(") + m_targetTableName + ")",
        [this](int argc, char **argv, char **azColName) {

        std::string tableColumnName = "";
        int colType = 0;

        for (int i = 0; i < argc; i++) {
            std::string colName = azColName[i];

            if (argv[i]) {
                if (colName == "name") {
                    tableColumnName = argv[i];
                } else if (colName == "type") {

                    std::string typeString = argv[i];
                    if (typeString == "INTEGER") {
                        colType = 1;
                    } else if (typeString == "TEXT") {
                        colType = 2;
                    }
                }
            }
        }
        m_columnTypeBufferMap[tableColumnName] = colType;
    }, m_lastErrorMessage)) {
        return false;
    }

    return
        m_db.exec(
            m_lastQuery,
            [this](int argc, char **argv, char **azColName){
            RowValue currentRowValue;

            for (int i = 0; i < argc; i++) {
                CellValue cellVal;

                auto cellTypeIt = m_columnTypeBufferMap.find(azColName[i]);
                if (cellTypeIt == m_columnTypeBufferMap.end()) {
                    LOG_WARNING("Not found column in type list:", azColName[i]);
                    continue;
                }

                try {
                    switch (cellTypeIt->second) {
                    case 1: cellVal = static_cast<int64_t>(std::stoi(argv[i])); break;
                    case 2: cellVal = std::string(argv[i]); break;
                    default: cellVal = {};
                    }
                } catch (std::invalid_argument& ex) { // For std::stoi
                    cellVal = {};
                }
                currentRowValue.push_back(cellVal);
            }
            m_querryRows.push_back(currentRowValue);
        }, m_lastErrorMessage);
}


}
