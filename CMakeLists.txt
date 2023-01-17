cmake_minimum_required(VERSION 3.21)

project(luabind)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(third_party)

add_library(luabind INTERFACE)
target_include_directories(luabind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(luabind INTERFACE lua_headers)

enable_testing()
add_subdirectory(tests)
