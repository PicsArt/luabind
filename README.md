## Introduction
Luabind is a small header only library aiming to provide easy means to expose C++ functionality in Lua.
Currently it supports inheritance, binding of class member functions, member variables and global functions.

## Example
```cpp
class Account : public luabind::Object {
public:
    Account(int balance);
public:
    void credit(int amount);
    void debit(int amount);

private:
    int balance;
};

class SpecialAccount : public Account {
public:
    SpecialAccount(int balance, int overdraft);

private:
    int overdraft;
};
```

```cpp
lua_State* L = luaL_newstate();
luaL_openlibs(L);
luabind::class_<Account>(L, "Account")
    .constructor<int>("new")
    .function<&Account::credit>("credit")
    .function<&Account::debit>("debit")
    .property_readonly<&Account::balance>("balance");

luabind::class_<SpecialAccount, Account>(L, "SpecialAccount")
    .constructor<int, int>("new")
    .property_readonly<&SpecialAccount::overdraft>("overdraft");

luaL_dostring(L, R"--(
    a = Account:new(10)
    a:credit(3)
    a:debit(2)
    s = SpecialAccount:new(50, 10)
    s:credit(20)
)--");
```
## How to install, confugure, build and run

Ordinary prcedure when developing and testing `luabind` library.

```sh
git clone https://github.com/PicsArt/luabind.git

git submodule update --recursive --init

cmake --preset luabind

cmake --build --preset luabind

ctest --preset unit_tests
```

There are some specifications when you want to use `luabind` as submodule. As a library it consist of submodules such as: `lua` and `googletest`. You can encounter to issues like `'multiple definition of googletest'` etc.. To avoid this conflicts on CMake level variables are defined to prevent such problems. 

| CMake variable | Purpose |
| ------ | ------ |
| LUA_LIB_NAME (STRING) | variable to link specific library (luabind_lua is set as default) |
| INCLUDE_LUA_LIB_WITH_EXTERN_C (BOOL) | variable to add/remove extern "C" when including lua headers |
| LUABIND_TEST (BOOL) | variable for inclusion/exclusion of test into build phase |


## Command line build steps

- To use custom `lua` library one can run with the following command.
```sh
cmake --preset luabind -DLUA_LIB_NAME=[your-custom-lua-library]
```
> Note: `-DLUA_LIB_NAME=luabind_lua` is set by default.

- To exclude `luabind` tests and `googletest` library from build phase run the following command.
```sh
cmake --preset luabind -DLUABIND_TEST=OFF
```
> Note: `-DLUABIND_TEST=ON` is set by default.

- To wrap/unwrap lua includes with `extern "C"` use the variable to do so.
```sh
cmake --preset luabind -DINCLUDE_LUA_LIB_WITH_EXTERN_C=OFF
```
> Note: `-DINCLUDE_LUA_LIB_WITH_EXTERN_C=ON` is set by default.

## Integrate to CMake

In this example `luabind` is used as submodule.
```
    set(INCLUDE_LUA_LIB_WITH_EXTERN_C ON)
    set(LUA_LIB_NAME lua)
    set(LUABIND_TEST OFF)

    add_subdirectory(luabind)
```


