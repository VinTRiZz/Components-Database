#include "sqlitedatabase.h"
#include "sqlitequery.h"

#include <vector>
#include <list>
#include <map>

#include <sqlite3.h>

#include <Components/Logger/Logger.h>

namespace SQLiteAdapter
{

static int callback(void *data, int argc, char **argv, char **azColName) {

    auto querryCallback = *reinterpret_cast< std::function<void (int argc, char **argv, char **azColName)>* >(data);
    RowValue row;
    querryCallback(argc, argv, azColName);
    return 0;
}

struct SQLiteDatabase::Impl
{
    // Handify
    std::string dbPath;
    std::string lastErrorMessage;

    // SQLite C library things
    sqlite3 *db {nullptr};
};

SQLiteDatabase::SQLiteDatabase() :
    d {new Impl}
{

}

SQLiteDatabase::~SQLiteDatabase()
{

}

void SQLiteDatabase::setDatabase(const std::string &filePath)
{
    d->dbPath = filePath;
}

bool SQLiteDatabase::open()
{
    if (sqlite3_open(d->dbPath.c_str(), &d->db)) {
        d->lastErrorMessage = sqlite3_errmsg(d->db);
        return false;
    }

    return true;
}

bool SQLiteDatabase::open(const std::string &filePath)
{
    d->dbPath = filePath;
    return open();
}

void SQLiteDatabase::close()
{
    sqlite3_close(d->db);
}

bool SQLiteDatabase::save()
{
    close();
    return open(d->dbPath);
}

SQLiteQuery SQLiteDatabase::query()
{
    SQLiteQuery result(*this);
    return result;
}

std::string SQLiteDatabase::lastError() const
{
    return d->lastErrorMessage;
}

bool SQLiteDatabase::exec(const std::string &queryStr, const std::function<void (int, char **, char **)> addRowCallback, std::string& queryErrorMessage)
{
    LOG_EMPTY("EXECUTING QUERY:\n", queryStr.c_str(), "\n");

    char *errorMessage {nullptr};

    auto pCallback = &addRowCallback;

    // Execute SQL statement
    if (sqlite3_exec(d->db, queryStr.c_str(), callback, (void*)pCallback, &errorMessage) != SQLITE_OK) {
        queryErrorMessage = errorMessage;
        sqlite3_free(errorMessage);
        LOG_ERROR("EXECUTE ERROR");
        return false;
    }
    LOG_OK("EXECUTE SUCCEED");
    return true;
}

}
