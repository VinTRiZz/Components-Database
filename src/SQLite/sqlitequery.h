#ifndef SQLITEQUERY_H
#define SQLITEQUERY_H

/*
 ******* EXAMPLE OF USAGE ********

SQLiteAdapter::SQLiteDatabase db;

LOG_INFO("Opening DB");
if (!db.open("adapter_testDB.db")) {
    LOG_CRITICAL("Open db error:", db.lastError());
    return -1;
}

LOG_INFO("Getting query");
auto query = db.query();

LOG_INFO("Creating table");
query.createTable("testT", { "id INTEGER", "txt TEXT" });
LOG_INFO("Created |", query.lastQuery());

LOG_INFO("Adding values");
if (!query.add("testT", {1, "Aboba text"})) {
    LOG_ERROR("Error adding:", query.lastError());
    LOG_DEBUG("Query:", query.lastQuery());
    return 2;
}
LOG_INFO("Added |", query.lastQuery());

LOG_INFO("Selecting values");
if (!query.select("testT", {"txt"})) {
    LOG_ERROR("Error selecting:", query.lastError());
    LOG_DEBUG("Query:", query.lastQuery());
    return 3;
}
LOG_INFO("Selected |", query.lastQuery());

while (query.next()) {
    if (!query.columnCount()) {
        LOG_ERROR("No columns");
        continue;
    }
    LOG_EMPTY("COLUMN DATA:", query.columnName(0), query.value(0));
}
return 0;
*/


#include <memory>
#include <vector>
#include <list>
#include <string>
#include <map>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

namespace SQLiteAdapter
{

typedef boost::optional<boost::variant<std::string, int64_t> > InputCellValue;
typedef boost::optional<boost::variant<std::string, int64_t> > CellValue;
typedef std::vector<CellValue> RowValue;

class InputCellValueVisitor
    : public boost::static_visitor<std::string>
{
public:

    std::string operator()(std::string input) const
    {
        return "'" + input + "'";
    }

    std::string operator()(int64_t input) const
    {
        return std::to_string(input);
    }
};

class SQLiteDatabase;

class SQLiteQuery
{
public:
    SQLiteQuery() = delete;
    SQLiteQuery(SQLiteDatabase& db);
    SQLiteQuery(const SQLiteQuery& other);

    bool exec(const std::string& queryStr);

    bool createTable(const std::string& tableName,
                     const std::vector<std::string>& columns,
                     const std::vector<std::pair<std::string, std::string> > foreignKeys = {});

    bool dropTable(const std::string& tableName);

    bool select(const std::string& tableName,
                const std::vector<std::string>& columns,
                const std::string& criteriaStr = "");

    bool add   (const std::string& tableName,
                const std::vector<InputCellValue> &values);

    bool update(const std::string& tableName,
                const std::vector<std::string>& columns,
                const std::vector<InputCellValue>& values,
                const std::string& criteriaStr = "");

    bool remove(const std::string& tableName,
                const std::string& criteriaStr = "");

    int16_t indexOf(const std::string& columnName);
    std::string columnName(int16_t columnIndex);

    std::size_t rowCount();
    int16_t columnCount();

    CellValue value(int16_t index);
    CellValue value(const std::string& columnName);
    void goNext();

    std::string lastError() const;
    std::string lastQuery() const;

private:
    SQLiteDatabase &m_db;

    std::string m_lastErrorMessage;
    std::string m_lastQuery;

    std::string m_targetTableName;
    std::map<std::string, int> m_columnTypeBufferMap;
    std::vector<std::string> m_queryColumns;

    std::list<RowValue> m_querryRows;
    decltype(m_querryRows)::iterator m_currentValueIt;

    bool defaultExec();
};

}

#endif // SQLITEQUERY_H
