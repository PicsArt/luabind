
include(CTest)

add_executable(base_class_binding base_class_binding.cpp)
target_link_libraries(base_class_binding luabind ${LUA_LIB_NAME} gtest_main)
add_test(NAME base_class_binding_test COMMAND base_class_binding)

add_executable(binding binding.cpp)
target_link_libraries(binding luabind ${LUA_LIB_NAME} gtest_main)
add_test(NAME binding_test COMMAND binding)

add_executable(const_binding const_binding.cpp)
target_link_libraries(const_binding luabind ${LUA_LIB_NAME} gtest_main)
add_test(NAME const_binding_test COMMAND const_binding)

