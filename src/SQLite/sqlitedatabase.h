#pragma once

#include <string>
#include <memory>

namespace Database
{

class SQLiteExecutor;
class SQLiteDatabase
{
public:
    SQLiteDatabase();
    ~SQLiteDatabase();

    static void setThreadsafe();

    // Для создания БД в RAM, укажите путь :memory:
    bool setDatabase(const std::string& filePath);
    bool isValid() const;
    std::string getLastError() const;

private:
    struct Impl;
    std::shared_ptr<Impl> d;

    friend class SQLiteExecutor;
    void* createConnection(SQLiteExecutor* requestingExecutor);
    void  startReadMode(void* connectionHandler);
    void  startWriteMode(void* connectionHandler);
    void  removeConnection(SQLiteExecutor* requestingExecutor);
};

}
