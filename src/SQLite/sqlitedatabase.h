#pragma once

#include <string>
#include <memory>

namespace Database
{

class SQLiteQuery;
class SQLiteDatabase
{
public:
    SQLiteDatabase();
    ~SQLiteDatabase();

    static void setThreadsafe();

    void setDatabase(const std::string& filePath);

    bool open(const std::string& filePath = {});
    bool isOpen() const;
    void close();

    bool save();

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    std::string lastError() const;

private:
    struct Impl;
    std::shared_ptr<Impl> d;
};

}
