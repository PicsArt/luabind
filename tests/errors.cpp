#include "lua_test.hpp"

#include <memory>

class String : public luabind::Object {
public:
    std::string str;

public:
    String(const std::string_view s)
        : str(s) {}

    void set(const std::string& s) {
        str = s;
    }

    const std::string& get() {
        return str;
    }

    void copyFrom(const std::shared_ptr<String> from) {
        str = from->str;
    }

    void copy(const String& from) {
        str = from.str;
    }
};

class Numeric : public luabind::Object {
public:
    void boolean(bool) {}

    void integral(int) {}

    void floating(float) {}
};

class StrLuaTest : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<String>(L, "String")
            .constructor<const std::string_view>("new")
            .construct_shared<const std::string_view>("create")
            .function<&String::set>("set")
            .function<&String::get>("get")
            .property<&String::str>("str")
            .function<&String::copy>("copy")
            .function<&String::copyFrom>("copyFrom");

        luabind::class_<Numeric>(L, "Numeric")
            .construct_shared<>("create")
            .function<&Numeric::boolean>("boolean")
            .function<&Numeric::integral>("integral")
            .function<&Numeric::floating>("floating");

        EXPECT_EQ(lua_gettop(L), 0);
    }
};

TEST_F(StrLuaTest, MirrorErrors) {
    runExpectingError(R"--(
        u = Numeric:new()
        a = String:new(u)
    )--",
                      "Argument at 2 has invalid type. Expecting 'string', but got 'userdata'.");

    runExpectingError(
        R"--(
        a = String:new('aaa')
        b = String:create('bbb')
        a:copyFrom(b) -- ok
        a:copyFrom('abcd') -- should result in error
    )--",
        "Argument at 2 has invalid type. Expecting user_data of type 'String', but got lua type 'string'");

    runExpectingError(R"--(
        a = String:new('aaa')
        b = String:new('bbb')
        a:copyFrom(b)
    )--",
                      "Argument at 2 is not a shared_ptr.");

    runExpectingError(R"--(
        a = String:new('aaa')
        b = Numeric:create()
        a:copyFrom(b)
    )--",
                      "Argument at 2 has invalid type. Expecing 'String' but got 'Numeric'.");

    runExpectingError(
        R"--(
        a = String:new('aaa')
        a:copy(555)
    )--",
        "Argument at 2 has invalid type. Expecting user_data of type 'String', but got lua type 'number'");

    runExpectingError(
        R"--(
        a = String:new('aaa')
        b = Numeric:new()
        a:copy(b)
    )--",
        "Argument at 2 has invalid type. Expecting 'String' but got 'Numeric'.");

    runExpectingError(
        R"--(
        u = Numeric:new()
        b:boolean(true)
        b:boolean(false)
        b:boolean(1)
    )--",
        "Argument at 2 has invalid type. Expecting 'boolean', but got 'number'.");

    runExpectingError(
        R"--(
        u = Numeric:new()
        b:integral(1)
        b:integral('abc')
    )--",
        "Argument at 2 has invalid type. Expecting 'integer', but got 'string'.");

    runExpectingError(
        R"--(
        u = Numeric:new()
        b:integral(1.5)
    )--",
        "Argument at 2 has invalid type. Expecting 'integer', but got 'number'.");

    runExpectingError(
        R"--(
        u = Numeric:new()
        b:floating(1.5)
        b:floating('abc')
    )--",
        "Argument at 2 has invalid type. Expecting 'number', but got 'string'.");
}
