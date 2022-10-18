set(LUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)

file(GLOB lua_header_files ${LUA_SOURCE_DIR}/*.h)
file(GLOB lua_sources ${LUA_SOURCE_DIR}/*.c)
list(REMOVE_ITEM lua_sources "${LUA_SOURCE_DIR}/lua.c" "${LUA_SOURCE_DIR}/luac.c" "${LUA_SOURCE_DIR}/onelua.c"  "${LUA_SOURCE_DIR}/ltests.c")

add_library(lua_headers INTERFACE ${lua_header_files})
add_library(lua STATIC ${lua_sources})

target_include_directories(lua_headers SYSTEM INTERFACE ${LUA_SOURCE_DIR})

target_link_libraries(lua lua_headers)
