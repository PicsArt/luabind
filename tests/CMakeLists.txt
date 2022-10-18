
include(CTest)

add_executable(binding binding.cpp)
target_link_libraries(binding luabind lua gtest_main)

add_test(NAME binding_test COMMAND binding)