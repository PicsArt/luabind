if(${LUA_LIB_NAME} STREQUAL luabind_lua)
    add_subdirectory(lua)
endif()

if(LUABIND_TEST)
    add_subdirectory(googletest)
endif()
