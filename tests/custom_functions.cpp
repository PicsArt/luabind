#include "lua_test.hpp"

class IntWrapper : public luabind::Object {
public:
    IntWrapper(int value)
        : _value(value) {}

    int get() const {
        return _value;
    }

public:
    static int toLuaTable(lua_State* L) {
        int argumentCount = lua_gettop(L);
        EXPECT_EQ(argumentCount, 1);

        const auto& data = luabind::value_mirror<IntWrapper>::from_lua(L, 1);
        lua_newtable(L);
        lua_pushstring(L, "value");
        lua_pushinteger(L, data.get());
        lua_settable(L, -3);

        return 1;
    }

private:
    int _value;
};

class CustomFunctionTest : public LuaTest {
protected:
    void SetUp() override {
        luabind::class_<IntWrapper>(L, "IntWrapper")
            .constructor<int>("new")
            .function("toTable", IntWrapper::toLuaTable);

        EXPECT_EQ(lua_gettop(L), 0);
    }
};

TEST_F(CustomFunctionTest, CustomFunction) {
    auto value = runWithResult<int>(R"--(
        obj = IntWrapper:new(13)
        objTable = obj:toTable()
        return objTable["value"]
    )--");

    EXPECT_EQ(value, 13);
}
