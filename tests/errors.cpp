#include "lua_test.hpp"

#include <memory>

class String : public luabind::Object {
public:
    std::string str;

public:
    String(const std::string_view s)
        : str(s) {}

    static String createString(const std::string_view s) {
        return String(s);
    }

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

    void customError(bool thr) {
        if (thr) {
            throw std::runtime_error("Custom std::error");
        } else {
            throw "unknown";
        }
    }
};

class Numeric : public luabind::Object {
public:
    void boolean(bool) {}

    void integral(int) {}

    void floating(float) {}
};

Numeric toNumeric(const String&) {
    return Numeric {};
}

class StrLuaTest : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<String>(L, "String")
            .constructor<const std::string_view>("new")
            .construct_shared<const std::string_view>("create")
            .class_function<&String::createString>("createString")
            .function<&String::set>("set")
            .function<&String::get>("get")
            .property<&String::str>("str")
            .function<&String::copy>("copy")
            .function<&String::copyFrom>("copyFrom")
            .function<&String::customError>("customError");

        luabind::class_<Numeric>(L, "Numeric")
            .construct_shared<>("create")
            .function<&Numeric::boolean>("boolean")
            .function<&Numeric::integral>("integral")
            .function<&Numeric::floating>("floating");

        luabind::function<&toNumeric>(L, "toNumeric");

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

TEST_F(StrLuaTest, WrapperErrors) {
    runExpectingError(
        R"--(
        s = String:new()
    )--",
        "Invalid number of arguments, should be 1, but 0 were given.");

    runExpectingError(
        R"--(
        s = String:new('abc', 1)
    )--",
        "Invalid number of arguments, should be 1, but 2 were given.");

    runExpectingError(
        R"--(
        s = String:create()
    )--",
        "Invalid number of arguments, should be 1, but 0 were given.");

    runExpectingError(
        R"--(
        s = String:create('abc', 1)
    )--",
        "Invalid number of arguments, should be 1, but 2 were given.");

    runExpectingError(
        R"--(
        s = String:createString()
    )--",
        "Invalid number of arguments, should be 1, but 0 were given.");

    runExpectingError(
        R"--(
        s = String:createString('abc', 1)
    )--",
        "Invalid number of arguments, should be 1, but 2 were given.");

    runExpectingError(
        R"--(
        s = String:new('abc')
        s:set()
    )--",
        "Invalid number of arguments, should be 1, but 0 were given.");

    runExpectingError(
        R"--(
        s = String:createString('abc')
        s:set('def', 2, 3)
    )--",
        "Invalid number of arguments, should be 1, but 3 were given.");

    runExpectingError(
        R"--(
        s = String:create('abc')
        s:customError(true)
    )--",
        "Custom std::error");

    runExpectingError(
        R"--(
        s = String:create('abc')
        s:customError(false)
    )--",
        "Unknown exception while trying to call C function from Lua.");

    runExpectingError(
        R"--(
        s = String:create('abc')
        n = toNumeric()
    )--",
        "Invalid number of arguments, should be 1, but 0 were given.");

    runExpectingError(
        R"--(
        s = String:create('abc')
        n = toNumeric(s)
        n = toNumeric(s, 2)
    )--",
        "Invalid number of arguments, should be 1, but 2 were given.");
}
