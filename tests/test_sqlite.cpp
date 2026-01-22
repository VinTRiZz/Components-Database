#include <gtest/gtest.h>

#include "../src/SQLite/sqlitedatabase.h"

TEST() {
    Database::SQLiteDatabase db;
    if (!db.setDatabase(":memory:")) {
        LOG_ERROR("Failed to open / create database. Error text:", db.getLastError());
        return 1;
    }

    if (!db.isValid()) {
        LOG_ERROR("Failed to open / create database, not valid. Error text:", db.getLastError());
        return 1;
    }

    Database::SQLiteTable table(db);
    table.setTable("test_table");

    std::list<Database::SQLiteTable::ColumnInfo> cols;

    Database::SQLiteTable::ColumnInfo tmpc;
    tmpc.name = "id";
    tmpc.type = Database::CT_INTEGER;
    tmpc.isPrimaryKey = true;
    cols.push_back(tmpc);

    tmpc = {};
    tmpc.name = "name";
    tmpc.type = Database::CT_TEXT;
    tmpc.defaultValue = "Unnamed";
    cols.push_back(tmpc);

    tmpc = {};
    tmpc.name = "description";
    tmpc.type = Database::CT_TEXT;
    tmpc.defaultValue = "No description provided";
    cols.push_back(tmpc);

    if (table.isTableExist()) {
        table.drop();
        table.setTable("test_table");
    }

    table.beginTransaction();
    if (table.create(cols)) {
        table.commitTransaction();
        LOG_OK("Transaction commited");
    } else {
        table.rollbackTransaction();
        LOG_ERROR("Transaction failed to commit:", table.getLastError());
    }

    for (int i = 0; i < 10; ++i) {
        if (!table.addRow({
            {"name",        std::string("Item ") + std::to_string(i)},
            {"description", std::string("Description of item no ") + std::to_string(i)}
        })) {
            LOG_ERROR("Failed to add row:", table.getLastError());
            return 1;
        }
    }

    if (!table.updateRow(
        {
            {"name",        std::string("Changed item")},
            {"description", std::string("My custom description")}
        },
        "id = 5")) {
        LOG_ERROR("Failed to update row:", table.getLastError());
        return 1;
    }

    if (!table.removeRow("id = 4")) {
        LOG_ERROR("Failed to remove row:", table.getLastError());
        return 1;
    }

    Common::SignalDecorator sigg;
    auto begTime = std::chrono::high_resolution_clock::now();

    std::thread testTh([&table, &sigg](){
        sigg.waitSignal();
        auto rows1 = table.getRows();
        LOG_DEBUG("1 operation completed");
        for (auto& r : rows1) {
            LOG_INFO("1 Row data:", Database::cellDataToString(r[0]), Database::cellDataToString(r[1]), Database::cellDataToString(r[2]));
        }
    });

    std::thread testTh2([&table, &sigg](){
        sigg.waitSignal();
        auto rows2 = table.getRows();
        LOG_DEBUG("2 operation completed");
        for (auto& r : rows2) {
            LOG_INFO("2 Row data:", Database::cellDataToString(r[0]), Database::cellDataToString(r[1]), Database::cellDataToString(r[2]));
        }
    });

    std::thread testTh3([&table, &sigg](){
        sigg.waitSignal();
        auto rows3 = table.getRows();
        LOG_DEBUG("3 operation completed");
        for (auto& r : rows3) {
            LOG_INFO("3 Row data:", Database::cellDataToString(r[0]), Database::cellDataToString(r[1]), Database::cellDataToString(r[2]));
        }
    });

    tmpc = {};
    tmpc.name = "ref_id";
    tmpc.type = Database::CT_INTEGER;
    tmpc.referedColumn = "id";
    tmpc.referenceDeleteAction = "CASCADE";
    tmpc.referenceUpdateAction = "CASCADE";

    LOG_DEBUG("Waiting...");
    sigg.emitForAll();
    if (testTh.joinable()) {
        testTh.join();
    }
    if (testTh2.joinable()) {
        testTh2.join();
    }
    if (testTh3.joinable()) {
        testTh3.join();
    }
    LOG_DEBUG("Wait complete");

    auto endTime = std::chrono::high_resolution_clock::now();
    LOG_INFO("Time elapsed:", std::chrono::duration_cast<std::chrono::microseconds>(endTime - begTime).count());

    table.beginTransaction();
    if (!table.addColumn(tmpc)) {
        LOG_ERROR("Failed to add column:", table.getLastError());
        table.rollbackTransaction();
        return 1;
    } else {
        table.commitTransaction();
    }

    table.beginTransaction();
    if (!table.removeColumn("ref_id")) {
        LOG_ERROR("Failed to remove column:", table.getLastError());
        table.rollbackTransaction();
        return 1;
    } else {
        table.commitTransaction();
    }

    LOG_OK("App exited normally");
    return 0;
}
