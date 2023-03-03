#include "lua_test.hpp"

#include <iostream>
#include <memory>
#include <string>

class Account : public luabind::Object {
public:
    static int count;
    static int flag;

public:
    Account()
        : balance(0) {
        ++count;
    }

    Account(int b)
        : balance(b) {
        ++count;
    }

    ~Account() {
        --count;
    };

    Account(const Account& r)
        : balance(r.balance) {
        ++count;
    }

    Account(Account&& r)
        : balance(r.balance) {
        ++count;
    }

    int getBalance() const {
        return balance;
    }

    void setBalance(int b) {
        balance = b;
    }

    virtual void service() {
        Account::flag = 1;
    }

public:
    int balance;
};

int Account::count = 0;
int Account::flag = 0;

class SpecialAccount : public Account {
public:
    void service() override {
        Account::flag = 2;
    }

    void setLimit(int l) {
        limit = l;
    }

public:
    int limit = 10;
};

class AccountLuaTest : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<Account>(L, "Account")
            .constructor<int>("newWithInt")
            .construct_shared<>("makeShared")
            .construct_shared<int>("makeSharedWithInt")
            .function<&Account::getBalance>("getBalance")
            .function<&Account::setBalance>("setBalance")
            .function<&Account::service>("service")
            .property<&Account::balance>("balance");

        luabind::class_<SpecialAccount, Account>(L, "SpecialAccount")
            .construct_shared<>("makeShared")
            .property_readonly<&SpecialAccount::limit>("limit")
            .function<&SpecialAccount::setLimit>("setLimit");

        luabind::function<&createAccount>(L, "createAccount");

        luabind::function<&setAccount>(L, "setAccount");
        luabind::function<&setSpecialAccount>(L, "setSpecialAccount");

        EXPECT_EQ(lua_gettop(L), 0);
    }

    void TearDown() override {
        account.reset();
        special_account.reset();
    }

    static std::shared_ptr<Account> createAccount(bool special) {
        return special ? std::make_shared<SpecialAccount>() : std::make_shared<Account>();
    }

    static void setAccount(std::shared_ptr<Account> acc) {
        account = std::move(acc);
    }

    static void setSpecialAccount(const std::shared_ptr<SpecialAccount>& acc) {
        special_account = acc;
    }

protected:
    static std::shared_ptr<Account> account;
    static std::shared_ptr<SpecialAccount> special_account;
};

std::shared_ptr<Account> AccountLuaTest::account;
std::shared_ptr<SpecialAccount> AccountLuaTest::special_account;

TEST_F(AccountLuaTest, CtorDtor) {
    EXPECT_EQ(Account::count, 0);
    int r = run(R"--(
        a1 = Account:new()
        a2 = Account:newWithInt(7)
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(Account::count, 2);

    const Account& a = runWithResult<const Account&>(R"--(
        return a2
    )--");
    EXPECT_EQ(a.balance, 7);
    EXPECT_EQ(Account::count, 2);

    r = run(R"--(
        a1 = nil
        collectgarbage()
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(Account::count, 1);

    r = run(R"--(
        a2 = nil
        collectgarbage()
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(Account::count, 0);
}

TEST_F(AccountLuaTest, VirtualFunctionCall) {
    EXPECT_EQ(Account::flag, 0);
    int r = run(R"--(
        a = createAccount(false)
        a:service()
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(Account::flag, 1);
    r = run(R"--(
        a = createAccount(true)
        a:service();
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(Account::flag, 2);
}

TEST_F(AccountLuaTest, FunctionsAndProperties) {
    int r = run(R"--(
        a = Account:makeShared()
        assert(a.balance == 0)
        a.balance = 1
        assert(a.balance == 1)
        a:setBalance(2)
        assert(a.balance == 2)
        assert(a:getBalance() == 2)
        local status, err = pcall(function() print(a.limit) end)
        assert(not status)
        setAccount(a)

        sa = SpecialAccount:makeShared()
        assert(sa.balance == 0)
        sa.balance = 1
        assert(sa.balance == 1)
        sa:setBalance(2)
        assert(sa.balance == 2)
        assert(sa:getBalance() == 2)
        assert(sa.limit == 10)

        local status, err = pcall(function() sa.limit = 2 end)
        assert(not status)
        sa:setLimit(5)
        setSpecialAccount(sa)
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(account->balance, 2);
    EXPECT_EQ(special_account->balance, 2);
    EXPECT_EQ(special_account->limit, 5);
}

struct Base : luabind::Object {
    int b = 0;
};

struct Child1 : virtual Base {
    int c1 = 0;
};

struct Child2 : virtual Base {
    int c2 = 0;
};

struct Derived
    : Child1
    , Child2 {
    int d = 0;
};

void validatePointers(Derived* d, Child1* c1, Child2* c2, Base* b) {
    EXPECT_EQ(c1, static_cast<Child1*>(d));
    EXPECT_EQ(c2, static_cast<Child2*>(d));
    EXPECT_EQ(b, static_cast<Base*>(d));
}

class MultipleInheritance : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<Base>(L, "Base").property<&Base::b>("b");
        luabind::class_<Child1, Base>(L, "Child1").property<&Child1::c1>("c1");
        luabind::class_<Child2, Base>(L, "Child2").property<&Child2::c2>("c2");
        luabind::class_<Derived, Child1, Child2>(L, "Derived").property<&Derived::d>("d");

        luabind::function<&validatePointers>(L, "validatePointers");
    }
};

TEST_F(MultipleInheritance, PointerCorrection) {
    Derived d = runWithResult<Derived>(R"--(
        o = Derived:new()
        o.b = 1
        o.c1 = 2
        o.c2 = 3
        o.d = 4
        validatePointers(o, o, o, o)
        return o
    )--");
    EXPECT_EQ(d.b, 1);
    EXPECT_EQ(d.c1, 2);
    EXPECT_EQ(d.c2, 3);
    EXPECT_EQ(d.d, 4);
}

struct Unbound : luabind::Object {
    int i = 0;
};

Unbound create() {
    Unbound u;
    u.i = 1;
    return u;
}

void testUnboundByRef(Unbound& u) {
    EXPECT_EQ(u.i, 1);
    u.i = 2;
}

void testUnbound(Unbound u) {
    EXPECT_EQ(u.i, 2);
    u.i = 3;
}

void testUnboundByConstRef(const Unbound& u) {
    EXPECT_EQ(u.i, 2);
}

TEST_F(LuaTest, Unbound) {
    luabind::function<&create>(L, "create");
    luabind::function<&testUnboundByRef>(L, "testUnboundByRef");
    luabind::function<&testUnboundByConstRef>(L, "testUnboundByConstRef");
    luabind::function<&testUnbound>(L, "testUnbound");
    [[maybe_unused]] int r = run(R"--(
        u = create()
        testUnboundByRef(u)
        testUnbound(u)
        testUnboundByConstRef(u)
    )--");
}

Account* accountPtr = nullptr;

void testSharedPtr(const std::shared_ptr<Account>& a) {
    EXPECT_EQ(a.get(), accountPtr);
    EXPECT_EQ(a->balance, 10);
    a->balance = 11;
}

void testSharedPtrByValue(std::shared_ptr<Account> a) {
    EXPECT_EQ(a.get(), accountPtr);
    EXPECT_EQ(a->balance, 11);
    a->balance = 12;
}

void testSharedPtrWithRawPtr(Account* a) {
    EXPECT_EQ(a, accountPtr);
    EXPECT_EQ(a->balance, 12);
    a->balance = 13;
}

void testSharedPtrWithRef(Account& a) {
    EXPECT_EQ(&a, accountPtr);
    EXPECT_EQ(a.balance, 13);
    a.balance = 14;
}

void testSharedPtrWithConstRawPtr(const Account* a) {
    EXPECT_EQ(a, accountPtr);
    EXPECT_EQ(a->balance, 14);
}

void testSharedPtrWithConstRef(const Account& a) {
    EXPECT_EQ(&a, accountPtr);
    EXPECT_EQ(a.balance, 14);
}

void testSharedPtrWithCopy(Account a) {
    EXPECT_NE(&a, accountPtr);
    EXPECT_EQ(a.balance, 14);
}

TEST_F(AccountLuaTest, SharedPtr) {
    luabind::function<&testSharedPtr>(L, "testSharedPtr");
    luabind::function<&testSharedPtrByValue>(L, "testSharedPtrByValue");
    luabind::function<&testSharedPtrWithRawPtr>(L, "testSharedPtrWithRawPtr");
    luabind::function<&testSharedPtrWithRef>(L, "testSharedPtrWithRef");
    luabind::function<&testSharedPtrWithConstRawPtr>(L, "testSharedPtrWithConstRawPtr");
    luabind::function<&testSharedPtrWithConstRef>(L, "testSharedPtrWithConstRef");
    luabind::function<&testSharedPtrWithCopy>(L, "testSharedPtrWithCopy");
    auto a = runWithResult<std::shared_ptr<Account>>(R"--(
        a = Account:makeSharedWithInt(10)
        return a;
    )--");
    accountPtr = a.get();
    int r = run(R"--(
        testSharedPtr(a)
        testSharedPtrByValue(a)
        testSharedPtrWithRawPtr(a)
        testSharedPtrWithRef(a)
        testSharedPtrWithConstRawPtr(a)
        testSharedPtrWithConstRef(a)
        testSharedPtrWithCopy(a)
        a = nil
        collectgarbage()
    )--");
    EXPECT_EQ(r, LUA_OK);
    EXPECT_EQ(a.use_count(), 1);
}
