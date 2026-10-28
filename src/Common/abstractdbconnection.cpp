#include "abstractdbconnection.hpp"

#include <algorithm>

namespace Database {

AbstractConnection::AbstractConnection(const std::string &appName, const std::string &connectionName) :
    m_appName { appName },
    m_connectionName{ connectionName } {

}

bool AbstractConnection::setAppName(const std::string &appName)
{
    m_appName = appName;
    return true;
}

std::string AbstractConnection::getAppName() const
{
    return m_appName;
}

bool AbstractConnection::setName(const std::string &conName)
{
    m_connectionName = conName;
    return true;
}

std::string AbstractConnection::getName()
{
    return m_connectionName;
}

bool AbstractConnection::setServer(const std::string &address, uint16_t port)
{
    m_address = address;
    m_port = port;
    return true;
}

std::string AbstractConnection::getServer() const
{
    return m_address;
}

uint16_t AbstractConnection::getServerPort() const
{
    return m_port;
}

bool AbstractConnection::setDatabase(const std::string &databaseName)
{
    m_databaseName = databaseName;
    return true;
}

std::string AbstractConnection::getDatabase() const
{
    return m_databaseName;
}

bool AbstractConnection::setUser(const std::string &username, const std::string &password)
{
    m_username = username;
    m_password = password;
    return true;
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

bool AbstractConnection::containInjection(const std::string_view &queryStr) const
{
    // 1. Stacked queries
    size_t semicolonPos = queryStr.find(';');
    while (semicolonPos != std::string_view::npos) {
        bool hasMoreContent = false;
        for (size_t i = semicolonPos + 1; i < queryStr.size(); ++i) {
            if (!std::isspace(static_cast<unsigned char>(queryStr[i]))) {
                hasMoreContent = true;
                break;
            }
        }
        if (hasMoreContent) {
            return true;
        }
        semicolonPos = queryStr.find(';', semicolonPos + 1);
    }

    // 2. Search for comments outside of quotes
    auto isInsideStringLiteral = [&](size_t pos) -> bool {
        bool inSingleQuote = false;
        bool inDoubleQuote = false;
        for (size_t i = 0; i < pos; ++i) {
            if (queryStr[i] == '\'') {
                if (inSingleQuote && i + 1 < pos && queryStr[i + 1] == '\'') {
                    ++i;
                } else {
                    inSingleQuote = !inSingleQuote;
                }
            } else if (queryStr[i] == '"') {
                inDoubleQuote = !inDoubleQuote;
            }
        }
        return inSingleQuote || inDoubleQuote;
    };

    const std::array<std::string_view, 3> commentMarkers = { "--", "/*", "#" };
    for (const auto &marker : commentMarkers) {
        size_t pos = queryStr.find(marker);
        while (pos != std::string_view::npos) {
            if (!isInsideStringLiteral(pos)) {
                return true;
            }
            pos = queryStr.find(marker, pos + 1);
        }
    }

    // 3. Check basic suspicious keyword combinations
    std::string lowerQuery;
    lowerQuery.reserve(queryStr.size());
    std::transform(queryStr.begin(), queryStr.end(), std::back_inserter(lowerQuery),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const std::array<std::string_view, 4> suspiciousPatterns = {
        "union select",   // UNION-based
        "or 1=1",         // Auth
        "or '1'='1",      // Auth
        "'; --"           // Close request try
    };

    for (const auto &pattern : suspiciousPatterns) {
        if (lowerQuery.find(pattern) != std::string::npos) {
            return true;
        }
    }

    // 4. Non-balanced quotes
    auto hasUnbalancedQuotes = [](std::string_view s, char quoteChar) -> bool {
        int count = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == quoteChar) {
                if (i + 1 < s.size() && s[i + 1] == quoteChar) {
                    ++i;
                } else if (i > 0 && s[i - 1] == '\\') {
                    continue;
                } else {
                    ++count;
                }
            }
        }
        return count % 2 != 0;
    };

    if (hasUnbalancedQuotes(queryStr, '\'') || hasUnbalancedQuotes(queryStr, '"')) {
        return true;
    }

    return false;
}

} // namespace Database
