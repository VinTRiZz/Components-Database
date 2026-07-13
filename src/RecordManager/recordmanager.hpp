#pragma once

#include <optional>
#include <string>
#include <memory>
#include <stdexcept>

#include <Components/Database/Common.h>
#include <Components/Database/AbstractConnection.h>
#include <Components/Database/RecordBase.h>

#include <Components/ExtraClasses/Error.h>

namespace Database {

class RecordManager;
using RecordManagerPtr = std::shared_ptr<RecordManager>;

class RecordManager : public ExtraClasses::ErrorUserBase<ExtraClasses::ErrorBase>
{
    // CRUD operations (simple queries)
    enum class QueryType
    {
        Insert = 0,
        SelectOne,
        SelectAll,
        Delete,
        Update,
    };

public:

    // For outer initialization
    void setConnection(const AbstractConnectionPtr& pCon);
    AbstractConnectionPtr getConnection() const;

    template <typename IdT, bool isSync = true>
    std::optional<IdT> addRecord(const RecordBase<IdT>& iValue) {
        auto res = m_connection->executeQuery(makeSimpleQuery(QueryType::Insert, iValue));
        if (!res.has_value()) {
            m_error = m_connection->getError();
            return {};
        }
        m_error.reset();
        return std::get<IdT>(res.value()[0][iValue.getIdColumn().data()]);
    }

    template <typename IdT, bool isSync = true>
    bool updateRecord(const RecordBase<IdT>& iValue) {
        auto res = m_connection->executeQuery(makeSimpleQuery(QueryType::Update, iValue));
        if (!res.has_value()) {
            m_error = m_connection->getError();
            return false;
        }
        m_error.reset();
        return true;
    }

    template <typename IdT, bool isSync = true>
    bool removeRecord(const RecordBase<IdT>& iValue) {
        auto res = m_connection->executeQuery(makeSimpleQuery(QueryType::Delete, iValue));
        if (!res.has_value()) {
            m_error = m_connection->getError();
            return false;
        }
        m_error.reset();
        return true;
    }

    template <typename RecordT, bool isSync = true>
    std::optional<RecordT> getRecord(const typename RecordT::id_t& recordId) const {
        RecordT iValue;
        iValue.setId(recordId);
        auto res = m_connection->executeQuery(makeSimpleQuery(QueryType::SelectOne, iValue));
        if (!res.has_value()) {
            m_error = m_connection->getError();
            return {};
        }
        if (!res.initFromRecord(res.value())) {
            m_error = res.getError();
            return {};
        }
        m_error.reset();
        return iValue;
    }

    template <typename RecordT, bool isSync = true>
    std::vector<RecordT> getAllRecords() const {
        std::vector<RecordT> outputValues;
        RecordT singleValue;
        auto res = m_connection->executeQuery(makeSimpleQuery(QueryType::SelectAll, singleValue));
        if (!res.has_value()) {
            m_error = m_connection->getError();
            return {};
        }
        for (auto& rec : res.value()) {
            if (!singleValue.initFromRecord(rec)) {
                continue;
            }
            outputValues.push_back(singleValue);
        }
        m_error.reset();
        return outputValues;
    }

private:
    // Connection info
    AbstractConnectionPtr m_connection;

    // Value conversions for simplicity
    std::string recordToColumns(const DBRowNamed& rec) const;
    std::string recordToValues(const DBRowNamed& rec) const;
    std::string recordToValueAssignList(const DBRowNamed& rec) const;

    template <typename IdT>
    std::string makeSimpleQuery(QueryType typ, const RecordBase<IdT>& iRecord) const {
        auto recordV = iRecord.toRecord();
        switch (typ)
        {
        case QueryType::Insert:
            return std::string("INSERT INTO ") +
                iRecord.getTable().data() +
                + " (" + recordToColumns(recordV) + ") VALUES ("
                + recordToValues(recordV) + ") RETURNING " + iRecord.getIdColumn().data()
            ;
        case QueryType::SelectOne:
            return std::string("SELECT ")
                   + recordToColumns(recordV) + " FROM "
                   + iRecord.getTable().data() +
                   + " WHERE "
                   + iRecord.getIdColumn().data() + " = " + iRecord.getIdString()
                ;
        case QueryType::SelectAll:
            return std::string("SELECT ")
                   + recordToColumns(recordV) + " FROM "
                   + iRecord.getTable().data()
                   + " ORDER BY " + iRecord.getIdColumn().data() + " ASC"
                ;
        case QueryType::Delete:
            return std::string("DELETE FROM ") +
                   iRecord.getTable().data() +
                   + " WHERE "
                   + iRecord.getIdColumn().data() + " = " + iRecord.getIdString()
                ;
        case QueryType::Update:
            return std::string("UPDATE ") +
                   iRecord.getTable().data() +
                   + " SET " + recordToValueAssignList(recordV)
                   + " WHERE "
                   + iRecord.getIdColumn().data() + " = " + iRecord.getIdString()
                ;
        }
        throw std::invalid_argument("RecordManager: Invalid query type passed");
    }
};

} // namespace Database
