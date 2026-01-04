#pragma once

#include <stdint.h>

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <future> // Для асинхронных квери

#if __cplusplus >= 201703UL
#include <optional>
#include <variant>
#else
#error "SQLite adaptor don't work with C++ lower than 17 standard (optional and variant required)"
#endif


namespace Database
{

using DBCellInteger = std::optional<int64_t>;
using DBCellDouble = std::optional<double>;
using DBCellString = std::optional<std::string>;

typedef std::variant<DBCellString, DBCellInteger, DBCellDouble> DBCell;
typedef std::vector<DBCell> DBRow;

// TODO: Обработать все типы?
enum ColumnType : int {
    CT_UNDEFINED = -1,
    CT_TEXT, // Default type
    CT_INTEGER,
    CT_DOUBLE,
    CT_BYTES,
};
std::string columnTypeToText(ColumnType ct);
ColumnType columnTypeFromText(const std::string& ct);

std::string cellDataToString(const DBCell& cellData);
DBCell      createNullValue(ColumnType ct);
bool        cellDataIsNull(const DBCell& cellData);
}

