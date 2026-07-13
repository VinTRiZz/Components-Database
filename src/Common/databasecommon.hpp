#pragma once

#include <stdint.h>

#include <string>
#include <vector>
#include <map>

#if __cplusplus >= 201703UL
#include <optional>
#include <variant>
#else
#error "Database adaptors don't work with C++ lower than 17 standard (optional and variant required)"
#endif


namespace Database
{

/**
 * @brief The SupportedDBType enum Used to determine conversion issues
 */
enum SupportedDBType : uint8_t
{
    SDBT_SQLite = 0,
    SDBT_PSQL
};

// DB value maanging
using DBCellNull    = std::monostate;
using DBCellInteger = int64_t;
using DBCellDouble  = double;
using DBCellString  = std::string;

// DB row managing
using DBCell = std::variant<DBCellNull, DBCellString, DBCellInteger, DBCellDouble>;
using DBRow = std::vector<DBCell>;
using DBRowNamed = std::map<std::string, DBCell>;

// TODO: Add more type processing for SQLite and PSQL
enum ColumnType : int {
    CT_UNDEFINED = -1,

    // Common types
    CT_TEXT,
    CT_INTEGER,
    CT_REAL,
    CT_DOUBLE,
    CT_BYTE,

    // Extra types
    CT_BOOL,

    // PSQL
    CT_JSON,
    CT_JSON_B,
};
std::string columnTypeToText(ColumnType ct, SupportedDBType dbType = SDBT_SQLite);
ColumnType columnTypeFromText(const std::string& ct, SupportedDBType dbType = SDBT_SQLite);

// Utility functions
std::string cellValueToString(const DBCell& cellData, SupportedDBType dbType = SDBT_SQLite);
DBCell      createNullValue(ColumnType ct, SupportedDBType dbType = SDBT_SQLite);
bool        cellDataIsNull(const DBCell& cellData);

}

