#include "sqliteexecutor.hpp"

#include <Components/Logger/Logger.h>

#include <sqlite3.h>
#include <boost/scope_exit.hpp>

#include <map>

#include "sqlitedatabase.h"

static int DB_SQLITE_INTERNAL_QUERY_CALLBACK(void *data, int argc, char **argv, char **azColName)
{
    auto querryCallback = *reinterpret_cast< std::function<void (int argc, char **argv, char **azColName)>* >(data);
    Database::DBRow row;
    querryCallback(argc, argv, azColName);
    return 0;
}

namespace Database {

struct SQLiteExecutor::Impl
{
    // SQLite C library things
    sqlite3 *dbConnection {nullptr};
    sqlite3_stmt* preparedStmt {nullptr};

    std::string lastQuery;
    std::string queryPrepared;
    std::string lastErrorText;

    std::list<std::shared_future<bool> > asyncQueries;
};

SQLiteExecutor::SQLiteExecutor(SQLiteDatabase &db) :
    d {new Impl}
{
    d->dbConnection = static_cast<sqlite3*>(db.createConnection(this));
}

SQLiteExecutor::~SQLiteExecutor()
{
    if (isOpen()) {
        sqlite3_close(d->dbConnection);
    }
}

bool SQLiteExecutor::isOpen() const
{
    if (d->dbConnection == NULL) {
        return false;
    }

    int highwater = 0;
    int current = 0;
    int rc = sqlite3_db_status(d->dbConnection, SQLITE_DBSTATUS_LOOKASIDE_USED, &current, &highwater, 0);

    return (rc == SQLITE_OK) ? 1 : 0;
}

bool SQLiteExecutor::beginTransaction()
{
    auto res = exec("BEGIN TRANSACTION");
    if (!res) {
        d->lastErrorText = getLastError();
    }
    return res.has_value();
}

bool SQLiteExecutor::commitTransaction()
{
    auto res = exec("COMMIT");
    if (!res) {
        d->lastErrorText = getLastError();
    }
    return res.has_value();
}

bool SQLiteExecutor::rollbackTransaction()
{
    auto res = exec("ROLLBACK");
    if (!res) {
        d->lastErrorText = getLastError();
    }
    return res.has_value();
}

bool SQLiteExecutor::prepare(const std::string &queryStr) const
{
    // Простейшая защита выполнения
    if (std::count(queryStr.begin(), queryStr.end(), ';') > 0) {
        d->lastErrorText = "SQL injection detected";
        return false;
    }

    auto bindRes = sqlite3_prepare(d->dbConnection, queryStr.c_str(), -1, &d->preparedStmt, NULL);
    if (bindRes == SQLITE_OK) {
        d->queryPrepared = queryStr;
    } else {
        d->lastErrorText = sqlite3_errmsg(d->dbConnection);
        d->queryPrepared.clear();
        d->preparedStmt = nullptr;
    }
    return !d->queryPrepared.empty();
}

bool SQLiteExecutor::bind(int parameterNumber, const DBCell &value)
{
    if (std::holds_alternative<DBCellInteger>(value)) {
        return (sqlite3_bind_int(d->preparedStmt, parameterNumber, std::get<DBCellInteger>(value).value())
                == SQLITE_OK);
    } else if (std::holds_alternative<DBCellString>(value)) {
        auto cellTextData = std::get<DBCellString>(value);
        return (sqlite3_bind_text(d->preparedStmt, parameterNumber, cellDataToString(cellTextData).c_str(), cellTextData.value().size(), SQLITE_STATIC)
                == SQLITE_OK);
    }
    return false;
}

std::optional<std::vector<DBRow> > SQLiteExecutor::exec(const std::string &queryStr) const
{
    if (queryStr.empty() && d->queryPrepared.empty()) {
        d->lastErrorText = "Empty query";
        return std::nullopt;
    }

//    LOG_DEBUG("EXECUTING QUERY:\"", queryStr, "\"");
    if (!queryStr.empty()) {
        if (!prepare(queryStr)) {
            return std::nullopt;
        }
    }

    BOOST_SCOPE_EXIT(d) {
        sqlite3_finalize(d->preparedStmt);
        d->preparedStmt = nullptr;
    } BOOST_SCOPE_EXIT_END;


    // Determine column types
    std::map<int, ColumnType> columnTypes;
    auto colCount = sqlite3_column_count(d->preparedStmt);
    for (int i = 0; i < colCount; i++) {
//        const char* colName = sqlite3_column_name(d->preparedStmt, i);
        const char* colType = sqlite3_column_decltype(d->preparedStmt, i);

        columnTypes.emplace(std::make_pair(i, (colType == NULL ? ColumnType::CT_TEXT : columnTypeFromText(colType))));
    }

    // Harvest rows
    std::vector<DBRow> res;
    DBRow tmpRow(colCount);
    int callRes = sqlite3_step(d->preparedStmt);
    while (callRes == SQLITE_ROW) {
        for (int i = 0; i < colCount; ++i) {
            int columnType = sqlite3_column_type(d->preparedStmt, i);

            // Corner-case
            if (columnType == SQLITE_BLOB) {
                const void* blob = sqlite3_column_blob(d->preparedStmt, i);
                int blobSize = sqlite3_column_bytes(d->preparedStmt, i);
                if (blob && blobSize > 0) {
                    std::string hexStr;
                    const unsigned char* bytes = static_cast<const unsigned char*>(blob);
                    for (int j = 0; j < blobSize; j++) {
                        char hex[3];
                        sprintf(hex, "%02x", bytes[j]);
                        hexStr += hex;
                    }
                    tmpRow[i] = (DBCellString{hexStr});
                } else {
                    tmpRow[i] = (DBCellString{std::nullopt});
                }
                continue;
            }

            switch (columnType)
            {
            case SQLITE_NULL:
                tmpRow[i] = createNullValue(columnTypes[i]);
                break;

            case SQLITE_INTEGER:
                tmpRow[i] = (DBCellInteger{sqlite3_column_int64(d->preparedStmt, i)});
                break;

            case SQLITE_FLOAT:
                tmpRow[i] = (DBCellDouble{sqlite3_column_double(d->preparedStmt, i)});
                break;

            default: // for SQLITE_TEXT also
                auto colText = sqlite3_column_text(d->preparedStmt, i);
                if (colText == NULL) {
                    tmpRow[i] = (DBCellString{});
                } else {
                    tmpRow[i] = (DBCellString{reinterpret_cast<const char*>(colText)});
                }
            }
        }

        res.push_back(tmpRow);
        for (auto& v : tmpRow) {
            v = {};
        }
        callRes = sqlite3_step(d->preparedStmt);
    }

    // Check if error exist
    if (callRes != SQLITE_DONE) {
        d->lastErrorText = sqlite3_errmsg(d->dbConnection);
        return std::nullopt;
    }
    return res;
}

bool SQLiteExecutor::execAsync(const std::string &queryStr, const std::function<void (std::vector<DBRow> &&)> &execCallback)
{
    LOG_ERROR("Async execution not implemented");
    return false;
}

std::string SQLiteExecutor::getLastQuery() const
{
    return d->lastQuery;
}

std::string SQLiteExecutor::getLastError() const
{
    return d->lastErrorText;
}

} // namespace Database
