#include "sqlitedatabase.h"

#include <Components/Logger/Logger.h>

#include <sqlite3.h>

#include "sqliteexecutor.hpp"

namespace Database
{

struct SQLiteDatabase::Impl
{
    sqlite3* dbConnection {nullptr};

    std::string dbPath;
    std::string lastErrorMessage;
};

SQLiteDatabase::SQLiteDatabase() :
    d {new Impl}
{

}

SQLiteDatabase::~SQLiteDatabase()
{
    if (isOpen()) {
        close();
    }
}

void SQLiteDatabase::setThreadsafe()
{
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    sqlite3_initialize();
}

void SQLiteDatabase::setDatabase(const std::string &filePath)
{
    d->dbPath = filePath;
}

bool SQLiteDatabase::open(const std::string &filePath)
{
    if (!filePath.empty()) {
        d->dbPath = filePath;
    }

    if (d->dbPath.empty()) {
        d->lastErrorMessage = "Empty database file path";
        return false;
    }

    if (sqlite3_open(d->dbPath.c_str(), &d->dbConnection)) {
        d->lastErrorMessage = sqlite3_errmsg(d->dbConnection);
        return false;
    }

    return true;
}

bool SQLiteDatabase::isOpen() const
{
    if (d->dbConnection == NULL) {
        return false;
    }

    int highwater = 0;
    int current = 0;
    int rc = sqlite3_db_status(d->dbConnection, SQLITE_DBSTATUS_LOOKASIDE_USED, &current, &highwater, 0);

    return (rc == SQLITE_OK) ? 1 : 0;
}

void SQLiteDatabase::close()
{
    sqlite3_close(d->dbConnection);
}

bool SQLiteDatabase::save()
{
    if (!isOpen()) {
        d->lastErrorMessage = "Database is not open";
        return false;
    }
    close();
    return open(d->dbPath);
}

bool SQLiteDatabase::beginTransaction()
{
    auto executor = SQLiteExecutor(*this);
    auto res = executor.exec("BEGIN TRANSACTION");
    if (!res) {
        d->lastErrorMessage = executor.getError();
    }
    return res.has_value();
}

bool SQLiteDatabase::commitTransaction()
{
    auto executor = SQLiteExecutor(*this);
    auto res = executor.exec("COMMIT");
    if (!res) {
        d->lastErrorMessage = executor.getError();
    }
    return res.has_value();
}

bool SQLiteDatabase::rollbackTransaction()
{
    auto executor = SQLiteExecutor(*this);
    auto res = executor.exec("ROLLBACK");
    if (!res) {
        d->lastErrorMessage = executor.getError();
    }
    return res.has_value();
}

std::string SQLiteDatabase::lastError() const
{
    return d->lastErrorMessage;
}

void *SQLiteDatabase::getConnection() const
{
    return d->dbConnection;
}

}
