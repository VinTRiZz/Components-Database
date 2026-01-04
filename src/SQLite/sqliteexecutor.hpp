#pragma once

#include "databasecommon.hpp"

struct sqlite3;

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

    bool prepare(const std::string& queryStr);
    std::optional<std::vector<DBRow> > exec(const std::string& queryStr = {});
    bool execAsync(const std::string& queryStr, const std::function<void(std::vector<DBRow>&&)>& execCallback);

    std::string getLastQuery() const;
    std::string getError() const;

private:
    struct Impl;
    std::shared_ptr<Impl> d;
};

} // namespace Database

