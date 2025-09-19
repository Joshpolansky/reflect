# Generated file to provide targets when using the build tree
if(NOT TARGET reflect-json::reflect-json)
    add_library(reflect-json INTERFACE IMPORTED)
    set_target_properties(reflect-json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include"
    )
    add_library(reflect-json::reflect-json ALIAS reflect-json)
endif()
