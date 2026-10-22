#include <gtest/gtest.h>

#ifdef COMPONENTS_SW_database_sqlite

#include <Components/Logger/Logger.h>
#include <Components/Database/SQlite.h>
#include <Components/ExtraClasses/Utility/SignalDecorator.h>

#include <thread>

TEST(Components_Database, CommonUsing) {
    COMPLOG_SET_LOGSDIR(".");

    Database::SQLiteDatabase db;
    ASSERT_TRUE(db.setDatabase(":memory:")) << "Failed to open / create database. Error text: " << db.getLastError();
    ASSERT_TRUE(db.isValid()) << "Failed to open / create database, not valid. Error text: " << db.getLastError();

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
        COMPLOG_SYNC_OK("Transaction commited");
    } else {
        table.rollbackTransaction();
        COMPLOG_SYNC_ERROR("Transaction failed to commit:", table.getLastError());
    }

    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(table.addRow({
            {"name",        std::string("Item ") + std::to_string(i)},
            {"description", std::string("Description of item no ") + std::to_string(i)}
        })) << "Failed to add row:" << table.getLastError();
    }

    ASSERT_TRUE(table.updateRow(
        {
            {"name",        std::string("Changed item")},
            {"description", std::string("My custom description")}
        },
            "id = 5")) << "Failed to update row:" << table.getLastError();
    ASSERT_TRUE(table.removeRow("id = 4")) <<"Failed to remove row:" << table.getLastError();

    ExtraClasses::SignalDecorator sigg;
    auto begTime = std::chrono::high_resolution_clock::now();

    std::thread testTh([&table, &sigg](){
        sigg.waitSignal();
        auto rows1 = table.getRows();
        COMPLOG_SYNC_DEBUG("1 operation completed");
        for (auto& r : rows1) {
            COMPLOG_SYNC_INFO("1 Row data:", Database::cellValueToString(r[0]), Database::cellValueToString(r[1]), Database::cellValueToString(r[2]));
        }
    });

    std::thread testTh2([&table, &sigg](){
        sigg.waitSignal();
        auto rows2 = table.getRows();
        COMPLOG_SYNC_DEBUG("2 operation completed");
        for (auto& r : rows2) {
            COMPLOG_SYNC_INFO("2 Row data:", Database::cellValueToString(r[0]), Database::cellValueToString(r[1]), Database::cellValueToString(r[2]));
        }
    });

    std::thread testTh3([&table, &sigg](){
        sigg.waitSignal();
        auto rows3 = table.getRows();
        COMPLOG_SYNC_DEBUG("3 operation completed");
        for (auto& r : rows3) {
            COMPLOG_SYNC_INFO("3 Row data:", Database::cellValueToString(r[0]), Database::cellValueToString(r[1]), Database::cellValueToString(r[2]));
        }
    });

    tmpc = {};
    tmpc.name = "ref_id";
    tmpc.type = Database::CT_INTEGER;
    tmpc.referedColumn = "id";
    tmpc.referenceDeleteAction = "CASCADE";
    tmpc.referenceUpdateAction = "CASCADE";

    COMPLOG_SYNC_DEBUG("Waiting...");
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
    COMPLOG_SYNC_DEBUG("Wait complete");

    auto endTime = std::chrono::high_resolution_clock::now();
    COMPLOG_SYNC_INFO("Time elapsed:", std::chrono::duration_cast<std::chrono::microseconds>(endTime - begTime).count());

    table.beginTransaction();
    ASSERT_TRUE(table.addColumn(tmpc)) << "Failed to add column:" << table.getLastError();
    table.commitTransaction();

    table.beginTransaction();
    ASSERT_TRUE(table.removeColumn("ref_id")) << "Failed to remove column:" << table.getLastError();
    table.commitTransaction();
}

#endif // COMPONENTS_SW_database_sqlite