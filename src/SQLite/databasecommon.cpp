#include "databasecommon.hpp"

namespace Database {

std::string cellDataToString(const DBCell &cellData)
{
    if (std::holds_alternative<DBCellString>(cellData)) {
        return std::string("'") + (std::get<DBCellString>(cellData).has_value() ? std::get<DBCellString>(cellData).value() : "") + "'";
    }

    if (std::holds_alternative<DBCellInteger>(cellData)) {
        return (std::get<DBCellInteger>(cellData).has_value() ? std::to_string(std::get<DBCellInteger>(cellData).value()) : "");
    }

    return {};
}

std::string columnTypeToText(ColumnType ct)
{
    switch (ct)
    {
    case CT_UNDEFINED:
        return "[UNDEFINED_COLUMN_TYPE]";

    case CT_TEXT:       return "TEXT";
    case CT_INTEGER:    return "INTEGER";
    case CT_DOUBLE:     return "DOUBLE";
    case CT_BYTES:      return "BLOB";

    }
    return {};
}

ColumnType columnTypeFromText(const std::string &ct)
{
    auto uppercaseCt = ct;
    for (auto& c : uppercaseCt) {
        c = std::toupper(c);
    }

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
        return CT_BYTES;
    }

    return CT_UNDEFINED;
}

}
