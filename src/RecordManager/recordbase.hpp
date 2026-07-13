#pragma once

#include <string>
#include <algorithm>

#include <Components/Database/Common.h>

#include <Components/ExtraClasses/Error.h>

namespace Database {

/**
 * @brief The RecordBase class Basic class for converting from/to DB records
 */
template <typename IdT>
class RecordBase : public ExtraClasses::ErrorUserBase<ExtraClasses::ErrorBase> {
public:
    using id_t = IdT;
    using id_nullable_t = std::optional<IdT>;

    explicit RecordBase(const std::string& tableName, const std::string& idColumn = "id") {
        m_table = tableName;
        m_idColumnName = idColumn;
    }

    std::string_view getTable() const {
        return m_table;
    }
    std::string_view getIdColumn() const {
        return m_idColumnName;
    }

    virtual DBRowNamed toRecord() const {
        DBRowNamed res;
        res[m_idColumnName] = m_id;
        return res;
    }

    virtual bool initFromRecord(const DBRowNamed& iRecord) {
        auto idColIt = iRecord.find(m_idColumnName);
        if (iRecord.end() == idColIt) {
            m_error.setCode(ExtraClasses::ErrorCode_UNKNOWN_ERROR);
            m_error.setDetailText(std::string("No such column: ") + getIdColumn().data());
            return false;
        }
        m_id = idColIt->second;
        return true;
    }

    virtual void setId(const std::variant<std::monostate, id_t>& id) {
        std::visit([this](auto&& iId){
            m_id = iId;
        }, id);
    }
    std::optional<id_t> getId() const {
        if (!std::holds_alternative<id_t>(m_id)) {
            return std::get<id_t>(m_id);
        }
        return {};
    }
    virtual std::string getIdString() const { return cellValueToString(m_id); }

private:
    std::string m_table;
    std::string m_idColumnName;
    DBCell      m_id {};

protected:
    // Clean string from symbols like ', ;, etc.
    void fixStringValueIssues(std::string& inputStr) const {
        std::replace_if(inputStr.data(), inputStr.data() + inputStr.size(),
                        [](auto c){
            return (c == '\'') || (c == ';') || (c == '"');
        }, ' ');
    }
};

using RecordBaseI = RecordBase<DBCellInteger>;
using RecordBaseS = RecordBase<DBCellString>;

} // namespace Database
