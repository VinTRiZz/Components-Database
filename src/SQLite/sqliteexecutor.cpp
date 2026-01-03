#include "sqliteexecutor.hpp"

#include <Components/Logger/Logger.h>

#include <sqlite3.h>

static int callback(void *data, int argc, char **argv, char **azColName) {

    auto querryCallback = *reinterpret_cast< std::function<void (int argc, char **argv, char **azColName)>* >(data);
    Database::DBRow row;
    querryCallback(argc, argv, azColName);
    return 0;
}

namespace Database {

struct SQLiteExecutor::Impl
{
    // SQLite C library things
    sqlite3 *db {nullptr};

    std::string lastQuery;
    std::string queryPrepared;
    std::string lastErrorText;

    std::list<std::shared_future<bool> > asyncQueries;
};

SQLiteExecutor::SQLiteExecutor(sqlite3* db) :
    d {new Impl}
{
    d->db = db;
}

bool SQLiteExecutor::prepare(const std::string &queryStr)
{
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users WHERE age > ?;";
    int rc = sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL);

    // TODO: Write-up
//    if (rc == SQLITE_OK) {
//        sqlite3_bind_int(stmt, 1, 18);  // Bind parameter

//        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
//            // Process row
//            const char* name = (const char*)sqlite3_column_text(stmt, 0);
//            int age = sqlite3_column_int(stmt, 1);
//            printf("Name: %s, Age: %d\n", name, age);
//        }

//        sqlite3_finalize(stmt);
//    }
    d->queryPrepared = (rc == SQLITE_OK ? queryStr : std::string());
    return !d->queryPrepared.empty();
}

std::optional<std::vector<DBRow> > SQLiteExecutor::exec(const std::string &queryStr)
{
    if (queryStr.empty() && d->queryPrepared.empty()) {
        d->lastErrorText = "Empty query";
        return std::nullopt;
    }

    LOG_EMPTY("EXECUTING QUERY:\n", queryStr.c_str(), "\n");

    char *errorMessage {nullptr};
    if (d->queryPrepared.empty()) {
        d->lastQuery = queryStr;
    } else {
        d->lastQuery = d->queryPrepared;
    }

    // Execute SQL statement
    if (sqlite3_exec(d->db, queryStr.c_str(), NULL, NULL, &errorMessage) != SQLITE_OK) {
        d->lastErrorText = errorMessage;
        sqlite3_free(errorMessage);
        LOG_ERROR("EXECUTE ERROR:", d->lastErrorText);
        return {};
    }
    LOG_OK("EXECUTE SUCCEED");
    return {};
}

bool SQLiteExecutor::execAsync(const std::string &queryStr, const std::function<void (std::vector<DBRow> &&)> &execCallback)
{
    LOG_EMPTY("EXECUTING QUERY:\n", queryStr.c_str(), "\n");

    char *errorMessage {nullptr};
    d->lastQuery = queryStr;

    // Execute SQL statement
    if (sqlite3_exec(d->db, queryStr.c_str(), callback, (void*)&execCallback, &errorMessage) != SQLITE_OK) {
        d->lastErrorText = errorMessage;
        sqlite3_free(errorMessage);
        LOG_ERROR("EXECUTE ERROR", d->lastErrorText);
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
