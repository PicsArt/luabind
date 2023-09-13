set(LUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)

file(GLOB luabind_lua_header_files ${LUA_SOURCE_DIR}/*.h)
file(GLOB luabind_lua_sources ${LUA_SOURCE_DIR}/*.c)
list(REMOVE_ITEM luabind_lua_sources "${LUA_SOURCE_DIR}/lua.c" "${LUA_SOURCE_DIR}/luac.c" "${LUA_SOURCE_DIR}/onelua.c"  "${LUA_SOURCE_DIR}/ltests.c")

add_library(luabind_lua_headers INTERFACE ${luabind_lua_header_files})
add_library(luabind_lua STATIC ${luabind_lua_sources})

target_include_directories(luabind_lua_headers SYSTEM INTERFACE ${LUA_SOURCE_DIR})

target_link_libraries(luabind_lua PUBLIC luabind_lua_headers)
