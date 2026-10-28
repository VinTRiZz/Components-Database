COMPONENTS_CONFIGURE_COMPONENT(Database "1.0.0")
option(COMPONENTS_SW_database_sqlite "" OFF)
COMPONENTS_LINK_COMPONENT(Components-Database Common)
COMPONENTS_LINK_COMPONENT(Components-Database ExtraClasses)
COMPONENTS_LINK_COMPONENT(Components-Database Logger)
if (COMPONENTS_SW_database_sqlite)
    target_compile_definitions(Components-Database PRIVATE COMPONENTS_SW_database_sqlite)
    target_link_libraries(Components-Database PRIVATE sqlite3)
    COMPONENTS_LINK_COMPONENT(Components-Database Filework)
else()
    COMPONENTS_REMOVE_SOURCES(Components-Database
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SQLite/sqlitedatabase.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SQLite/sqlitedatabase.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SQLite/sqliteexecutor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SQLite/sqliteexecutor.hpp"
    )
endif()
COMPONENTS_ADD_COMPONENT_TEST(Database)
