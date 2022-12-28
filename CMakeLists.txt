cmake_minimum_required(VERSION 3.21)

project(luabind)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

# variable to use custom library
set(LUA_LIB_NAME luabind_lua CACHE STRING "lua library name")
message("luabind uses [ ${LUA_LIB_NAME} ] as lua library")

set(INCLUDE_LUA_LIB_WITH_EXTERN_C ON CACHE BOOL "flag to determine whether lua library is included with extern 'C'")
if(INCLUDE_LUA_LIB_WITH_EXTERN_C)
    add_compile_options(-DINCLUDE_LUA_LIB_WITH_EXTERN_C)
endif(INCLUDE_LUA_LIB_WITH_EXTERN_C)

# variable to include tests to build phase
set(LUABIND_TEST ON CACHE BOOL "variable to determine whether tests are included in build phase or not")

add_subdirectory(third_party)

add_library(luabind INTERFACE)
target_include_directories(luabind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(luabind INTERFACE ${LUA_LIB_NAME}_headers)

if(LUABIND_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
