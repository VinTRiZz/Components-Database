#pragma once

#include <functional>
#include <string>
#include <memory>

#include <Components/Database/Common.h>

#include <Components/ExtraClasses/Error.h>

namespace Database {

class AbstractConnection;
using AbstractConnectionPtr = std::shared_ptr<AbstractConnection>;

/**
 * @brief The AbstractConnection class Object to describe connection to a database
 * @note Some parameters of a connection have a straight depend on a db (SQLite, PSQL, etc.)
 */
class AbstractConnection : public ExtraClasses::ErrorUserBase<ExtraClasses::ErrorObject>
{
public:
    // Expect same args as execute() return
    using queryCallback_t = std::function<void(std::optional<std::vector<DBRowNamed> >&&)>;

    AbstractConnection() = default;
    AbstractConnection(const std::string& appName, const std::string& connectionName);

    // Connection properties
    bool        setAppName(const std::string& appName);
    std::string getAppName() const;

    bool        setName(const std::string& conName);
    std::string getName();

    bool        setServer(const std::string &address, uint16_t port);
    std::string getServer() const;
    uint16_t    getServerPort() const;

    virtual bool        setDatabase(const std::string &databaseName);
    std::string getDatabase() const;

    bool setUser(const std::string &username, const std::string &password);
    std::string getUsername() const;
    std::string getPassword() const;

    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;

    /**
     * @brief executeQuery  With empty argument, tries to execute prepared query
     * @param queryStr
     * @return              std::nullopt if query failed
     */
    virtual std::optional<std::vector<DBRowNamed> > executeQuery(const std::string& queryStr) = 0;

    /**
     * @brief executeQueryAsync Same as executeQuery, but calls queryCallback_t exit
     * @param queryStr
     * @param cbk
     */
    virtual bool executeQueryAsync(const std::string& queryStr, queryCallback_t&& cbk) = 0;

    virtual std::string cellDataToString(const DBCell &val) const;

    virtual bool prepare(const std::string &queryStr) = 0;
    virtual bool bind(int parameterNumber, const DBCell &value) = 0;
    virtual bool bind(const std::string_view parameterName, const DBCell& value) = 0;

private:
    std::string m_appName;
    std::string m_connectionName;
    std::string m_address;
    uint16_t    m_port;
    std::string m_databaseName;
    std::string m_username;
    std::string m_password;

protected:
    // Checks for basic SQL injection methods
    bool containInjection(const std::string_view& queryStr) const;
};

} // namespace Database
