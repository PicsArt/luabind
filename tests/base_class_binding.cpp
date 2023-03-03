#include "lua_test.hpp"

#include <memory>
#include <string_view>

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

    std::string_view childFunction() {
        return "FromDerived3";
    }
};

std::string_view usage(std::shared_ptr<Base> ptr) {
    return ptr->type();
}

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
        luabind::class_<Base>(L, "Base").function<&Base::type>("type");
        luabind::class_<Derived3, Base>(L, "Derived3")
            .construct_shared<>("makeShared")
            .function<&Derived3::childFunction>("childFunction");
        luabind::function<&create>(L, "create");
        luabind::function<&usage>(L, "usage");

        EXPECT_EQ(lua_gettop(L), 0);
    }
};

TEST_F(BaseClassBindingTest, BaseClassBindingTest) {
    int r = run(R"--(
        d1 = create("Derived1")
        assert(d1:type() == "Derived1")

        d2 = create("Derived2")
        assert(d2:type() == "Derived2")

        d3 = create("Derived3")
        assert(d3:type() == "Derived3")
        assert(d3:childFunction() == "FromDerived3")

        d3_concrete = Derived3:makeShared()
        assert(d3_concrete:type() == "Derived3")
        assert(d3_concrete:childFunction() == "FromDerived3")

        assert(usage(d3_concrete) == "Derived3")
        assert(usage(d1) == "Derived1")
        
    )--");

    EXPECT_EQ(r, LUA_OK);
}
