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
        col.name = cellDataToString(row[currentColNo++]);
        col.type = columnTypeFromText(std::get<DBCellString>(row[currentColNo++]).value());
        col.canBeNull = (std::get<DBCellInteger>(row[currentColNo++]).value() == 0);
        col.defaultValue = row[currentColNo++];
        col.isPrimaryKey = (std::get<DBCellInteger>(row[currentColNo++]).value() != 0);

        LOG_DEBUG("Col:",
                  col.name,
                  columnTypeToText(col.type),
                  col.canBeNull,
                  cellDataIsNull(col.defaultValue) ? "NULL" : cellDataToString(col.defaultValue),
                  col.isPrimaryKey);
        m_columns.push_back(col);
    }


//    // 2. Получаем информацию о внешних ключах
//    sql = "PRAGMA foreign_key_list(\"" + tableName + "\");";
//    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
//        // id (0), seq (1), table (2), from (3), to (4), on_update (5), on_delete (6)
//        while (sqlite3_step(stmt) == SQLITE_ROW) {
//            const char* from_col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
//            const char* to_col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
//            const char* on_delete = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

//            if (from_col && to_col) {
//                // Ищем столбец с именем from_col
//                for (auto& col : columns) {
//                    if (col.name == from_col) {
//                        col.referedColumn = to_col;
//                        if (on_delete) {
//                            col.referenceDeleteAction = on_delete;
//                        }
//                        break;
//                    }
//                }
//            }
//        }
//        sqlite3_finalize(stmt);
//    }

//    // 3. Получаем дополнительную информацию о PRIMARY KEY из pragma index_list/index_info
//    // (для AUTOINCREMENT и более точного определения PK)
//    sql = "PRAGMA index_list(\"" + tableName + "\");";
//    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
//        while (sqlite3_step(stmt) == SQLITE_ROW) {
//            // name (0), unique (1), origin (2), partial (3)
//            const char* origin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
//            if (origin && strcmp(origin, "pk") == 0) {
//                const char* index_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

//                // Получаем столбцы, входящие в этот PK индекс
//                std::string index_sql = "PRAGMA index_info(\"" + std::string(index_name) + "\");";
//                sqlite3_stmt* idx_stmt;

//                if (sqlite3_prepare_v2(db, index_sql.c_str(), -1, &idx_stmt, nullptr) == SQLITE_OK) {
//                    while (sqlite3_step(idx_stmt) == SQLITE_ROW) {
//                        // seqno (0), cid (1), name (2)
//                        const char* col_name = reinterpret_cast<const char*>(sqlite3_column_text(idx_stmt, 2));

//                        // Отмечаем найденный столбец как PK
//                        for (auto& col : columns) {
//                            if (col.name == col_name) {
//                                col.isPrimaryKey = true;
//                                break;
//                            }
//                        }
//                    }
//                    sqlite3_finalize(idx_stmt);
//                }
//            }
//        }
//        sqlite3_finalize(stmt);
//    }
}

// TODO: Разобраться, писал дипсик. 100% что-то не работает
std::optional<DBCell> SQLiteTable::parseDefaultValue(const char *sqlite_default, ColumnType col_type) {
    std::string default_str(sqlite_default);
    if (!sqlite_default || default_str.size() == 0) {
        return std::nullopt;
    }

    // Убираем кавычки для строковых значений
    if (default_str.front() == '\'' && default_str.back() == '\'') {
        default_str = default_str.substr(1, default_str.length() - 2);
        // Экранированные кавычки внутри строки
        size_t pos = 0;
        while ((pos = default_str.find("''", pos)) != std::string::npos) {
            default_str.replace(pos, 2, "'");
            pos += 1;
        }
        return DBCellString{default_str};
    }

    // Числовые значения и специальные ключевые слова
    if (col_type == ColumnType::CT_INTEGER || col_type == ColumnType::CT_DOUBLE) {
        // Попытка преобразовать в число
        try {
            if (default_str.find('.') != std::string::npos) {
                double val = std::stod(default_str);
                // Если нужен REAL в DBCell, преобразуем в строку
                return DBCellString{std::to_string(val)};
            } else {
                int64_t val = std::stoll(default_str);
                return DBCellInteger{val};
            }
        } catch (...) {
            // Если не число, возвращаем как строку
            return DBCellString{default_str};
        }
    }

    // Для остальных типов или если парсинг не удался
    return DBCellString{default_str};
}

}
