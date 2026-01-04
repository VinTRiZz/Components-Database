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
    d->dbConnection = static_cast<sqlite3*>(db.getConnection());
}

bool SQLiteExecutor::prepare(const std::string &queryStr) const
{
    auto bindRes = sqlite3_prepare_v2(d->dbConnection, queryStr.c_str(), -1, &d->preparedStmt, NULL);
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
    if (d->queryPrepared.empty()) {
        if (!prepare(queryStr)) {
            return std::nullopt;
        }
    }

    BOOST_SCOPE_EXIT(d) {
        sqlite3_finalize(d->preparedStmt);
        d->preparedStmt = nullptr;
    } BOOST_SCOPE_EXIT_END;

    LOG_EMPTY("EXECUTING QUERY:\"", queryStr, "\"");

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
    if (colCount == 0) {
        return res;
    }

    int callRes {SQLITE_DONE};
    while ((callRes = sqlite3_step(d->preparedStmt)) == SQLITE_ROW) {
        for (int i = 0; i < colCount; i++) {
            int columnType = sqlite3_column_type(d->preparedStmt, i);

            if (columnType == SQLITE_NULL) {
                if (columnTypes[i] == SQLITE_INTEGER) {
                    tmpRow.emplace_back(DBCellInteger{std::nullopt});
                } else {
                    tmpRow.emplace_back(DBCellString{std::nullopt});
                }
            }
            else if (columnType == SQLITE_INTEGER) {
                tmpRow.emplace_back(DBCellInteger{sqlite3_column_int64(d->preparedStmt, i)});
            }
            else if (columnType == SQLITE_FLOAT) {
                double value = sqlite3_column_double(d->preparedStmt, i);
                tmpRow.emplace_back(DBCellString{std::to_string(value)});
            }
            else if (columnType == SQLITE_TEXT) {
                const unsigned char* text = sqlite3_column_text(d->preparedStmt, i);
                tmpRow.emplace_back(DBCellString{std::string(reinterpret_cast<const char*>(text))});
            }
            else if (columnType == SQLITE_BLOB) {
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
                    tmpRow.emplace_back(DBCellString{hexStr});
                } else {
                    tmpRow.emplace_back(DBCellString{std::nullopt});
                }
            }
            else {
                const unsigned char* text = sqlite3_column_text(d->preparedStmt, i);
                tmpRow.emplace_back(DBCellString{text ? std::string(reinterpret_cast<const char*>(text)) : ""});
            }
        }

        res.push_back(std::move(tmpRow));
    }

    // Check if error exist
    if (callRes != SQLITE_DONE) {
        d->lastErrorText = sqlite3_errmsg(d->dbConnection);
        LOG_ERROR("EXECUTE FAILED:", d->lastErrorText);
        return std::nullopt;
    }
    LOG_OK("EXECUTE SUCCEED");
    return res;
}

bool SQLiteExecutor::execAsync(const std::string &queryStr, const std::function<void (std::vector<DBRow> &&)> &execCallback)
{
    LOG_EMPTY("EXECUTING QUERY:\"", queryStr, "\"");

    char *errorMessage {nullptr};
    d->lastQuery = queryStr;

    // Execute SQL statement
    if (sqlite3_exec(d->dbConnection, queryStr.c_str(), DB_SQLITE_INTERNAL_QUERY_CALLBACK, (void*)&execCallback, &errorMessage) != SQLITE_OK) {
        d->lastErrorText = errorMessage;
        sqlite3_free(errorMessage);
        LOG_ERROR("EXECUTE ERROR: \"", d->lastErrorText, "\"");
        return false;
    }
    LOG_OK("EXECUTE SUCCEED");
    return true;
}

std::string SQLiteExecutor::getLastQuery() const
{
    return d->lastQuery;
}

std::string SQLiteExecutor::getError() const
{
    return d->lastErrorText;
}

} // namespace Database
