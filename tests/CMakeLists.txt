
include(CTest)

add_executable(binding binding.cpp const_binding.cpp)
target_link_libraries(binding luabind lua gtest_main)

add_test(NAME binding_test COMMAND binding)
add_test(NAME const_binding_test COMMAND binding)
