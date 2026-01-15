#pragma once

#include "databasecommon.hpp"

namespace Common {
class AccessManager;
}

namespace Database {

class SQLiteDatabase;

class SQLiteExecutorError
{
    std::string m_errorText;
public:
    void setError(const std::string& errorText);
    std::string getError() const;
};

class SQLiteExecutor
{
public:
    SQLiteExecutor(SQLiteDatabase& db);
    ~SQLiteExecutor();

    bool isOpen() const;

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool prepare(const std::string& queryStr) const;
    bool bind(int parameterNumber, const DBCell& value);
    std::optional<std::vector<DBRow> > exec(const std::string& queryStr = {}, bool isReadOperation = false) const; // TODO: Определять "читателя" иначе?

    std::string getLastQuery() const;
    std::string getLastError() const;

private:
    struct Impl;
    std::shared_ptr<Impl> d;
};

} // namespace Database

