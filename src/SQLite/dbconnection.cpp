#include "dbconnection.hpp"

#include <thread>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <map>

#include <Components/Logger/Logger.h>

#include <sqlite3.h>

namespace Database::SQLite {

static int db_sqlite_internalcbk(void *data, int argc, char **argv, char **azColName)
{
    auto querryCallback = *reinterpret_cast< std::function<void (int argc, char **argv, char **azColName)>* >(data);
    querryCallback(argc, argv, azColName);
    return 0;
}

std::shared_ptr<sqlite3> DBConnection::createInternalConnection() {
    sqlite3* pCon {nullptr};
    if (sqlite3_open(getDatabase().c_str(), &pCon) != SQLITE_OK) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseConnectionCreateFailed);
        m_error.setDetailText(sqlite3_errmsg(pCon));
        sqlite3_close_v2(pCon);
        return {};
    }
    return std::shared_ptr<sqlite3>(pCon, [this](sqlite3* pCon){
        m_preparedStmt.reset();
        sqlite3_close_v2(pCon);
    });
}

void DBConnection::updateExecuteError()
{
    m_error.setCode(ExtraClasses::ErrorCode::DatabaseConnectionCreateFailed);
    m_error.setDetailText(sqlite3_errmsg(m_pCon.get()));
}

bool DBConnection::setDatabase(const std::string &databaseName)
{
    if (databaseName.empty()) {
        m_pCon.reset();
        AbstractConnection::setDatabase({});
        return true;
    }

    if (databaseName == ":memory:") {
        AbstractConnection::setDatabase(databaseName);
        m_pCon = createInternalConnection();
        return (nullptr != m_pCon);
    }

    if (!std::filesystem::exists(databaseName)) {
        std::ofstream ofile;
        ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        try {
            ofile.open(std::string(databaseName), std::ios_base::trunc);
        } catch (const std::exception& ex) {
            m_error.setCode(ExtraClasses::ErrorCode::SystemFileError);
            m_error.setDetailText(std::string("Create error: ") + ex.what());
            return false;
        }
    }
    m_error.reset();
    AbstractConnection::setDatabase(databaseName);
    m_pCon = createInternalConnection();
    return (nullptr != m_pCon);
}

bool DBConnection::beginTransaction()
{
    return executeQuery("BEGIN TRANSACTION").has_value();
}

bool DBConnection::commitTransaction()
{
    return executeQuery("COMMIT").has_value();
}

bool DBConnection::rollbackTransaction()
{
    return executeQuery("ROLLBACK").has_value();
}

std::optional<std::vector<DBRowNamed> > DBConnection::executeQuery(const std::string &queryStr)
{
    if (!m_pCon) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseQueryExecutionFailed);
        m_error.setDetailText("Connection not inited");
        return {};
    }
    std::vector<DBRowNamed> execRes;
    if (queryStr.empty() && m_preparedQuery.empty()) {
        m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
        m_error.setDetailText("Empty query");
        return {};
    }

    m_lastQuery = queryStr.empty() ? m_preparedQuery : queryStr;
    if (!queryStr.empty() && !prepare(queryStr)) {
        return {};
    }
    m_error.reset();

    // Determine column types
    std::map<int, ColumnType> columnTypes;
    std::map<int, std::string> columnNames;
    auto colCount = sqlite3_column_count(m_preparedStmt.get());
    for (int i = 0; i < colCount; i++) {
        columnNames[i] = sqlite3_column_name(m_preparedStmt.get(), i);

        const char* colType = sqlite3_column_decltype(m_preparedStmt.get(), i);
        columnTypes[i] = (colType == NULL ? ColumnType::CT_TEXT : columnTypeFromText(colType));
    }

    // Harvest rows
    int callRes = sqlite3_step(m_preparedStmt.get());
    while (SQLITE_ROW == callRes) {
        DBRowNamed tmpRow;
        for (int i = 0; i < colCount; ++i) {
            int columnType = sqlite3_column_type(m_preparedStmt.get(), i);

            // Corner-case
            if (columnType == SQLITE_BLOB) {
                const void* blob = sqlite3_column_blob(m_preparedStmt.get(), i);
                int blobSize = sqlite3_column_bytes(m_preparedStmt.get(), i);
                if (blob && blobSize > 0) {
                    std::string hexStr;
                    const unsigned char* bytes = static_cast<const unsigned char*>(blob);
                    for (int j = 0; j < blobSize; j++) {
                        char hex[3];
                        sprintf(hex, "%02x", bytes[j]);
                        hexStr += hex;
                    }
                    tmpRow[columnNames[i]] = (DBCellString{hexStr});
                } else {
                    tmpRow[columnNames[i]] = (DBCellString{});
                }
                continue;
            }

            switch (columnType)
            {
            case SQLITE_NULL:
                tmpRow[columnNames[i]] = createNullValue(columnTypes[i]);
                break;

            case SQLITE_INTEGER:
                tmpRow[columnNames[i]] = (DBCellInteger{sqlite3_column_int64(m_preparedStmt.get(), i)});
                break;

            case SQLITE_FLOAT:
                tmpRow[columnNames[i]] = (DBCellDouble{sqlite3_column_double(m_preparedStmt.get(), i)});
                break;

            default: // for SQLITE_TEXT also
                auto colText = reinterpret_cast<const char*>(sqlite3_column_text(m_preparedStmt.get(), i));
                auto textLen = sqlite3_column_bytes(m_preparedStmt.get(), i);
                DBCellString res;
                if (colText != NULL) {
                    res = DBCellString(colText, textLen);
                }
                tmpRow.emplace(columnNames[i], res);
            }
        }

        execRes.push_back(tmpRow);
        callRes = sqlite3_step(m_preparedStmt.get());
    }

    const auto expandedQuery = sqlite3_expanded_sql(m_preparedStmt.get());
    if (expandedQuery) {
        m_lastQuery = expandedQuery;
        sqlite3_free(expandedQuery);
    }

    // Check if error exist
    if (callRes != SQLITE_DONE) {
        updateExecuteError();
        m_preparedStmt.reset();
        return {};
    }
    m_preparedStmt.reset();
    return execRes;
}

bool DBConnection::executeQueryAsync(const std::string &queryStr, queryCallback_t &&cbk)
{
    if (!m_pCon) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseQueryExecutionFailed);
        m_error.setDetailText("Connection not inited");
        return false;
    }
    std::thread([this, queryStr, cbk = std::move(cbk)](){
        cbk(executeQuery(queryStr));
    }).detach();
    return true;
}

bool DBConnection::prepare(const std::string &queryStr)
{
    if (!m_pCon) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseQueryExecutionFailed);
        m_error.setDetailText("Connection not inited");
        return false;
    }
    if (containInjection(queryStr)) {
        m_error.setCode(ExtraClasses::ErrorCode::DatabaseQueryExecutionFailed);
        m_error.setDetailText("SQL injection detected");
        return false;
    }

    sqlite3_stmt* stmt {nullptr};
    auto bindRes = sqlite3_prepare_v2(m_pCon.get(), queryStr.c_str(), queryStr.size(), &stmt, NULL);
    if (bindRes == SQLITE_OK) {
        m_preparedQuery = queryStr;
        m_error.reset();
        m_preparedStmt = std::shared_ptr<sqlite3_stmt>(stmt, [](sqlite3_stmt* stmt){
            if (nullptr != stmt) {
                sqlite3_finalize(stmt);
            }
        });
    } else {
        updateExecuteError();
        m_preparedQuery.clear();
        m_preparedStmt.reset();
    }
    return (nullptr != m_preparedStmt);
}

bool DBConnection::bind(int parameterNumber, const DBCell &value)
{
    if (!m_preparedStmt) { return false; }

    // TODO: Work with BLOB and 16/32 text
    m_error.reset();
    if (std::holds_alternative<DBCellNull>(value)) {
        return (sqlite3_bind_null(m_preparedStmt.get(), parameterNumber) == SQLITE_OK);
    } else if (std::holds_alternative<DBCellInteger>(value)) {
        return (sqlite3_bind_int(m_preparedStmt.get(), parameterNumber, std::get<DBCellInteger>(value))
                == SQLITE_OK);
    } else if (std::holds_alternative<DBCellDouble>(value)) {
        return (sqlite3_bind_double(m_preparedStmt.get(), parameterNumber, std::get<DBCellDouble>(value))
                == SQLITE_OK);
    } else if (std::holds_alternative<DBCellString>(value)) {
        auto cellTextData = std::get<DBCellString>(value);
        return (sqlite3_bind_text(m_preparedStmt.get(), parameterNumber, cellTextData.c_str(), cellTextData.size(), SQLITE_TRANSIENT)
                == SQLITE_OK);
    }

    m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
    m_error.setDetailText("Invalid index or value to bind");
    return false;
}

bool DBConnection::bind(const std::string_view parameterName, const DBCell &value)
{
    if (!m_preparedStmt) { return false; }

    int index = sqlite3_bind_parameter_index(m_preparedStmt.get(), parameterName.data());
    if (index > 0) {
        return bind(index, value);
    }
    m_error.setCode(ExtraClasses::ErrorCode::LogicalInvalidValue);
    m_error.setDetailText("Invalid index or value to bind");
    return false;
}

std::string_view DBConnection::getLastQuery() const
{
    return m_lastQuery;
}

} // namespace SQLite::Database
