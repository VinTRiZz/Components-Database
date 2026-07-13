COMPONENTS_CONFIGURE_COMPONENT(Database
    sqlite3
)

COMPONENTS_LINK_COMPONENT(Database Logger)
COMPONENTS_LINK_COMPONENT(Database Common)
COMPONENTS_LINK_COMPONENT(Database ExtraClasses)

target_precompile_headers(Database PUBLIC
    "src/SQLite/sqlitedatabase.hpp"
    "src/SQLite/sqlitetable.hpp"
)

target_precompile_headers(Database PRIVATE
    "src/SQLite/sqliteexecutor.hpp"
)