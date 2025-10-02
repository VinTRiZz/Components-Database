#ifndef SQLITEDATABASE_H
#define SQLITEDATABASE_H

#include <memory>
#include <vector>

#include "sqlitequery.h"

namespace SQLiteAdapter
{

class SQLiteQuery;

class SQLiteDatabase
{
public:
    SQLiteDatabase();
    ~SQLiteDatabase();

    void setDatabase(const std::string& filePath);
    bool open();
    bool open(const std::string& filePath);
    void close();
    bool save();

    SQLiteQuery query();
    std::string lastError() const;

private:
    bool exec(const std::string &queryStr, const std::function<void (int argc, char **argv, char **azColName)> addRowCallback, std::string &queryErrorMessage);
    struct Impl;
    std::shared_ptr<Impl> d;

    friend class SQLiteQuery;
};

}

#endif // SQLITEDATABASE_H
