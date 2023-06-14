#include "lua_test.hpp"

class IntWrapper : public luabind::Object {
public:
    IntWrapper(int value)
        : _value(value) {}

    int get() const {
        return _value;
    }

public:
    static int toLuaTable(lua_State* state) {
        int argumentCount = lua_gettop(state);
        EXPECT_EQ(argumentCount, 1);

        const auto& data = luabind::value_mirror<IntWrapper>::from_lua(state, 1);
        lua_newtable(state);
        lua_pushstring(state, "value");
        lua_pushinteger(state, data.get());
        lua_settable(state, -3);

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
