cmake_minimum_required(VERSION 3.21)

project(luabind)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

# flags to control lua includes
set(LUA_LIB_NAME luabind_lua CACHE STRING "lua library name")
message(WARNING "luabind uses ${LUA_LIB_NAME} as lua library")

set(INCLUDE_LUA_LIB_WITH_EXTERN_C ON CACHE BOOL "flag to determine whether lua library is included with extern 'C'")
IF(INCLUDE_LUA_LIB_WITH_EXTERN_C)
	add_compile_options(-DINCLUDE_LUA_LIB_WITH_EXTERN_C)
ENDIF(INCLUDE_LUA_LIB_WITH_EXTERN_C)

add_subdirectory(third_party)

add_library(luabind INTERFACE)
target_include_directories(luabind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(luabind INTERFACE ${LUA_LIB_NAME}_headers)

enable_testing()
add_subdirectory(tests)
