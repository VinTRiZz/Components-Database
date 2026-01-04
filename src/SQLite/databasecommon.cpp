#include "databasecommon.hpp"

namespace Database {

std::string cellDataToString(const DBCell &cellData)
{
    if (std::holds_alternative<CellStringValue>(cellData)) {
        return std::string("'") + (std::get<CellStringValue>(cellData).has_value() ? std::get<CellStringValue>(cellData).value() : "") + "'";
    }

    if (std::holds_alternative<CellIntegerValue>(cellData)) {
        return (std::get<CellIntegerValue>(cellData).has_value() ? std::to_string(std::get<CellIntegerValue>(cellData).value()) : "");
    }

    return {};
}

}
