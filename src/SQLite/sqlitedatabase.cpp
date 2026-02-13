#include "sqlitedatabase.h"

#include <Components/Logger/Logger.h>
#include <Components/Common/AccessManager.h>

#include <sqlite3.h>

#include <set>
#include <fstream>
#include <filesystem>

#include "sqliteexecutor.hpp"

namespace Database
{

struct SQLiteDatabase::Impl
{
    std::string dbPath;
    std::string lastErrorMessage;

    std::set<SQLiteExecutor*>       executors;
    std::unique_lock<std::mutex>    writerMx;
    bool                            isAnyReading {false};

    Common::AccessManager accessManager;
};

SQLiteDatabase::SQLiteDatabase() :
    d {new Impl}
{

}

SQLiteDatabase::~SQLiteDatabase()
{

}

bool SQLiteDatabase::setDatabase(const std::string &filePath)
{
    // Corner-case для SQLite
    if (filePath == ":memory:") {
        d->dbPath = filePath;
        return true;
    }

    if (!std::filesystem::exists(std::filesystem::path(filePath).parent_path())) {
        d->lastErrorMessage = "Invalid path (file either it's directory not exists)";
        return false;
    }

    if (!std::filesystem::exists(filePath)) {
        std::ofstream oCreateFile(filePath);
        oCreateFile.close();
    }

    std::fstream oFile(filePath, std::ios_base::in | std::ios_base::out);
    if (!oFile.is_open()) {
        d->lastErrorMessage = "Invalid path (failed to create or open file)";
        return false;
    }
    oFile.close();

    d->dbPath = filePath;
    return true;
}

bool SQLiteDatabase::isValid() const
{
    sqlite3* pCon {nullptr};
    if (sqlite3_open(d->dbPath.c_str(), &pCon) != SQLITE_OK) {
        d->lastErrorMessage = sqlite3_errmsg(pCon);
        sqlite3_close(pCon);
        return false;
    }
    sqlite3_close(pCon);
    return true;
}

std::string SQLiteDatabase::getLastError() const
{
    return d->lastErrorMessage;
}

Common::AccessManager &SQLiteDatabase::getAccessManager()
{
    return d->accessManager;
}

void *SQLiteDatabase::createConnection(SQLiteExecutor* requestingExecutor)
{
    if (!d->accessManager.isWorking()) {
        d->accessManager.start();
    }

    sqlite3* pCon {nullptr};
    if (sqlite3_open(d->dbPath.c_str(), &pCon) != SQLITE_OK) {
        sqlite3_close(pCon);
        d->lastErrorMessage = sqlite3_errmsg(pCon);
        COMPLOG_ERROR("Connection", reinterpret_cast<uint64_t>(requestingExecutor) % 100'000, "failed to open");
        return nullptr;
    }

    d->executors.insert(requestingExecutor);
    COMPLOG_OK("Connection", reinterpret_cast<uint64_t>(requestingExecutor) % 100'000, "created");
    return pCon;
}

}
