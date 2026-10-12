# SQLite is provided by the system by default. If the hosting project set up a
# Conan configuration (through Components-CMake), prefer the Conan "sqlite3"
# package and its SQLite::SQLite3 target instead.
set(DATABASE_DEPENDENCY_LIBS sqlite3)

COMPONENTS_CONAN_IS_AVAILABLE(_database_conan_on)
if (_database_conan_on)
    COMPONENTS_CONAN_REQUIRE(SQLite3)
    if (TARGET SQLite::SQLite3)
        set(DATABASE_DEPENDENCY_LIBS SQLite::SQLite3)
    endif()
endif()

COMPONENTS_CONFIGURE_COMPONENT(Database
    ${DATABASE_DEPENDENCY_LIBS}
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