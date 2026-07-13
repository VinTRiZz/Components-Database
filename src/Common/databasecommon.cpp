#include "databasecommon.hpp"

namespace Database {

std::string columnTypeToText(ColumnType ct, SupportedDBType dbType)
{
    switch (ct)
    {
    case CT_UNDEFINED: return "[UNDEFINED_TYPE]";

    // Common types
    case CT_TEXT:   return "TEXT";
    case CT_INTEGER:return "INTEGER";
    case CT_REAL:   return "REAL";
    case CT_DOUBLE: return (dbType == SDBT_SQLite ? "REAL" : "DOUBLE PRECISION");
    case CT_BYTE:   return (dbType == SDBT_SQLite ? "BLOB" : "BYTEA");

    // Extra types
    case CT_BOOL:   return (dbType == SDBT_SQLite ? "INTEGER" : "BOOLEAN");

    // PSQL
    case CT_JSON:   return "JSON";
    case CT_JSON_B: return "JSONB";

    }
    return {};
}

ColumnType columnTypeFromText(const std::string &ct, SupportedDBType dbType)
{
    auto uppercaseCt = ct;
    for (auto& c : uppercaseCt) {
        c = std::toupper(c);
    }

    // TODO: Add more type conversions

    if (uppercaseCt == "INTEGER") { // Also can be INT_??
        return CT_INTEGER;
    }

    if (uppercaseCt == "TEXT") { // Also can be CHAR, CLOB
        return CT_TEXT;
    }

    if (uppercaseCt == "DOUBLE") { // Also can be REAL, FLOAT, DECIMAL
        return CT_DOUBLE;
    }

    if (uppercaseCt == "BLOB") {
        return CT_BYTE;
    }

    return CT_UNDEFINED;
}

std::string cellValueToString(const DBCell &cellData, SupportedDBType dbType)
{
    if (std::holds_alternative<DBCellNull>(cellData)) {
        return "NULL";
    }

    if (std::holds_alternative<DBCellString>(cellData)) {
        return std::string("'") + std::get<DBCellString>(cellData) + "'";
    }

    if (std::holds_alternative<DBCellInteger>(cellData)) {
        return std::to_string(std::get<DBCellInteger>(cellData));
    }

    if (std::holds_alternative<DBCellDouble>(cellData)) {
        return std::to_string(std::get<DBCellDouble>(cellData));
    }

    return {};
}

DBCell createNullValue(ColumnType ct, SupportedDBType dbType)
{
    switch (ct)
    {
    case CT_UNDEFINED:  throw std::invalid_argument("Invalid column type (undefined)");

    case CT_TEXT:       return DBCellString();
    case CT_INTEGER:    return DBCellInteger();
    case CT_DOUBLE:     return DBCellDouble();

#warning "TODO: Add types"

    default: return DBCellString();
    }
    return DBCellDouble();
}

bool cellDataIsNull(const DBCell &cellData)
{
    return (cellValueToString(cellData).size() == 2);
}

}
