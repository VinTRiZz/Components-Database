#pragma once

#include <string>
#include <memory>

namespace ExtraClasses {
class AccessManager;
}

namespace Database
{

class SQLiteExecutor;
class SQLiteDatabase
{
public:
    SQLiteDatabase();
    ~SQLiteDatabase();

    // Для создания БД в RAM, укажите путь :memory:
    bool setDatabase(const std::string& filePath);
    bool isValid() const;
    std::string getLastError() const;

private:
    struct Impl;
    std::shared_ptr<Impl> d;

    friend class SQLiteExecutor;
    void* createConnection(SQLiteExecutor* requestingExecutor);
    ExtraClasses::AccessManager& getAccessManager();
};

}
