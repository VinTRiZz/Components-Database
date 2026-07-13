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
class AbstractConnection : public ExtraClasses::ErrorUserBase<ExtraClasses::ErrorBase>
{
public:
    AbstractConnection() = default;
    AbstractConnection(const std::string& appName, const std::string& connectionName);

    // Connection properties
    void        setAppName(const std::string& appName);
    std::string getAppName() const;

    void        setName(const std::string& conName);
    std::string getName(const std::string& conName);

    void        setServer(const std::string &address, uint16_t port);
    std::string getServer() const;
    uint16_t    getServerPort() const;

    void        setDatabase(const std::string &databaseName);
    std::string getDatabase() const;

    void setUser(const std::string &username, const std::string &password);
    std::string getUsername() const;
    std::string getPassword() const;

    // Execution working
    using queryCallback_t = std::function<void(std::vector<DBRowNamed>&&, const std::string&)>; // arguments: Values and error
    virtual std::optional<std::vector<DBRowNamed> > executeQuery(const std::string& queryStr, bool isSync = true) const = 0;
    virtual void executeQueryAsync(const std::string& queryStr, queryCallback_t&& cbk) const = 0;
    virtual std::string cellDataToString(const DBCell &val) const;

private:
    std::string m_appName;
    std::string m_connectionName;
    std::string m_address;
    uint16_t    m_port;
    std::string m_databaseName;
    std::string m_username;
    std::string m_password;
};

} // namespace Database
