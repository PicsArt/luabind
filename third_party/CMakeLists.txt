if(${LUABIND_LUA_LIB_NAME} STREQUAL luabind_lua)
    add_subdirectory(lua)
endif()

if(LUABIND_TESTS)
    add_subdirectory(googletest)
endif()
