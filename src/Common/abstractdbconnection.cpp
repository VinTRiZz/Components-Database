#include "abstractdbconnection.hpp"

namespace Database {

AbstractConnection::AbstractConnection(const std::string &appName, const std::string &connectionName) :
    m_appName { appName },
    m_connectionName{ connectionName } {

}

void AbstractConnection::setAppName(const std::string &appName)
{
    m_appName = appName;
}

std::string AbstractConnection::getAppName() const
{
    return m_appName;
}

void AbstractConnection::setName(const std::string &conName)
{
    m_connectionName = conName;
}

std::string AbstractConnection::getName(const std::string &conName)
{
    return m_connectionName;
}

void AbstractConnection::setServer(const std::string &address, uint16_t port)
{
    m_address = address;
    m_port = port;
}

std::string AbstractConnection::getServer() const
{
    return m_address;
}

uint16_t AbstractConnection::getServerPort() const
{
    return m_port;
}

void AbstractConnection::setDatabase(const std::string &databaseName)
{
    m_databaseName = databaseName;
}

std::string AbstractConnection::getDatabase() const
{
    return m_databaseName;
}

void AbstractConnection::setUser(const std::string &username, const std::string &password)
{
    m_username = username;
    m_password = password;
}

std::string AbstractConnection::getUsername() const
{
    return m_username;
}

std::string AbstractConnection::getPassword() const
{
    return m_password;
}

std::string AbstractConnection::cellDataToString(const DBCell &val) const
{
    return cellValueToString(val);
}

} // namespace Database
