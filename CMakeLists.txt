if (COMPONENTS_IS_ENABLED_QT)
    COMPONENTS_CONFIGURE_COMPONENT(Database
        Qt5::Core
        sqlite3
    )
else()
    COMPONENTS_CONFIGURE_COMPONENT(Database
        sqlite3
    )
endif()

COMPONENTS_LINK_COMPONENT(Database Logger)
COMPONENTS_LINK_COMPONENT(Database Common)
