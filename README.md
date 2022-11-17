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
