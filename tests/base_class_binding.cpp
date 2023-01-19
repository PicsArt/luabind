#include <memory>
#include <string_view>

#include <luabind/bind.hpp>

#include <gtest/gtest.h>

#include "lua_test.hpp"

class Base : public luabind::Object {
public:
    virtual std::string_view type() const = 0;
    virtual ~Base() = default;
};

class Derived1 : public Base {
public:
    std::string_view type() const override {
        return "Derived1";
    }
};

class Derived2 : public Base {
    std::string_view type() const override {
        return "Derived2";
    }
};

class Derived3 : public Derived1 {
public:
    std::string_view type() const override {
        return "Derived3";
    }
};

std::shared_ptr<Base> create(std::string_view name) {
    if (name == "Derived1") {
        return std::make_shared<Derived1>();
    } else if (name == "Derived2") {
        return std::make_shared<Derived2>();
    } else if (name == "Derived3") {
        return std::make_shared<Derived3>();
    } else {
        return nullptr;
    }
}

class BaseClassBindingTest : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<Base>(L, "StringBox").function<&Base::type>("type");
        luabind::function<&create>(L, "create");

        EXPECT_EQ(lua_gettop(L), 0);
    }
};

TEST_F(BaseClassBindingTest, BaseClassBindingTest) {
    int r = run(R"--(
        d1 = create("Derived1")
        print(d1:type())

        d2 = create("Derived2")
        print(d2:type())

        d3 = create("Derived3")
        print(d3:type())
        
    )--");

    EXPECT_EQ(r, LUA_OK);
}
