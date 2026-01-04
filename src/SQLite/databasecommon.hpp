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

using CellStringValue = std::optional<std::string>;
using CellIntegerValue = std::optional<int64_t>;

typedef std::variant<CellStringValue, CellIntegerValue> DBCell;
typedef std::vector<DBCell> DBRow;

std::string cellDataToString(const DBCell& cellData);

}

