#pragma once

#include <Components/Database/AbstractConnection.h>

struct sqlite3;
struct sqlite3_stmt;

namespace Database::SQLite {

/**
 * @brief The DBConnection class SQLite database connection
 */
class DBConnection : public AbstractConnection
{
public:
    // AbstractConnection interface
    bool setDatabase(const std::string &databaseName) override;

    // AbstractConnection interface
    virtual bool beginTransaction() override;
    virtual bool commitTransaction() override;
    virtual bool rollbackTransaction() override;

    // AbstractConnection interface
    std::optional<std::vector<DBRowNamed> > executeQuery(const std::string &queryStr = {}) override;
    bool executeQueryAsync(const std::string &queryStr, queryCallback_t &&cbk) override;

    // AbstractConnection interface
    bool prepare(const std::string &queryStr) override; // Bind allowed examples:  "?", ":myparam". "?2"
    bool bind(int parameterNumber, const DBCell &value) override;
    bool bind(const std::string_view parameterName, const DBCell& value) override;

    std::string_view getLastQuery() const;

private:
    std::shared_ptr<sqlite3>        m_pCon {};
    std::shared_ptr<sqlite3_stmt>   m_preparedStmt {};
    std::string     m_preparedQuery;
    std::string     m_lastQuery;

    // Main method
    std::shared_ptr<sqlite3> createInternalConnection();

    // Error getting
    void updateExecuteError();
};

}
