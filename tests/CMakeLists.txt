
include(CTest)

add_executable(binding binding.cpp)
target_link_libraries(binding luabind ${LUA_LIB_NAME} gtest_main)

add_test(NAME binding_test COMMAND binding)
