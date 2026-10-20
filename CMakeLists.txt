COMPONENTS_CONFIGURE_COMPONENT(Database "1.0.0"
    sqlite3
)

COMPONENTS_LINK_COMPONENT(Components-Database Logger)
COMPONENTS_LINK_COMPONENT(Components-Database Common)
COMPONENTS_LINK_COMPONENT(Components-Database ExtraClasses)

target_precompile_headers(Components-Database PUBLIC
    "src/SQLite/sqlitedatabase.hpp"
    "src/SQLite/sqlitetable.hpp"
)
