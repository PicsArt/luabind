set(LUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)

file(GLOB LUABIND_LUA_SOURCES ${LUA_SOURCE_DIR}/*.c)
list(REMOVE_ITEM LUABIND_LUA_SOURCES "${LUA_SOURCE_DIR}/lua.c" "${LUA_SOURCE_DIR}/luac.c" "${LUA_SOURCE_DIR}/onelua.c"  "${LUA_SOURCE_DIR}/ltests.c")

add_library(luabind_lua STATIC ${LUABIND_LUA_SOURCES})
target_include_directories(luabind_lua SYSTEM INTERFACE ${LUA_SOURCE_DIR})
