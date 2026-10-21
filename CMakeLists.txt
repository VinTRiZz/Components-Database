COMPONENTS_CONFIGURE_COMPONENT(Database "1.0.0"
    sqlite3
)
target_precompile_headers(Components-Database PUBLIC
    "src/SQLite/sqlitedatabase.hpp"
    "src/SQLite/sqlitetable.hpp"
)
COMPONENTS_CCR_CONAN_IS_CONFIGURED(COMPONENTS_CONAN_CONFIGURED)
if (COMPONENTS_CONAN_CONFIGURED)
    COMPONENTS_CCR_CONAN_REQUIRE(SQLite3)
endif()
COMPONENTS_LINK_COMPONENT(Components-Database Core)
COMPONENTS_LINK_COMPONENT(Components-Database Common)
COMPONENTS_LINK_COMPONENT(Components-Database ExtraClasses)
COMPONENTS_LINK_COMPONENT(Components-Database Logger)
COMPONENTS_ADD_COMPONENT_TEST(Database)
