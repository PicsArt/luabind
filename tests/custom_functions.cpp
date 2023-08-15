#include "lua_test.hpp"

class IntWrapper : public luabind::Object {
public:
    IntWrapper(int value = 7)
        : _value(value) {}

    int get() const {
        return _value;
    }

    static IntWrapper create(int value) {
        return IntWrapper(value);
    }

public:
    static int constructor(lua_State* L) {
        int args = lua_gettop(L) - 1; // first argument is the metatable
        if (args == 0) {
            return luabind::value_mirror<IntWrapper>::to_lua(L);
        } else {
            return luabind::value_mirror<IntWrapper>::to_lua(L, luabind::value_mirror<int>::from_lua(L, 2));
        }
    }

    static int toLuaTable(lua_State* L) {
        int argumentCount = lua_gettop(L);
        EXPECT_EQ(argumentCount, 1);

        return luaTable(L);
    }

    static int luaTable(lua_State* L) {
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
            .constructor("new", &IntWrapper::constructor)
            .class_function<IntWrapper::create>("create")
            .function("toTable", IntWrapper::toLuaTable)
            .property_readonly("table", IntWrapper::luaTable);

        EXPECT_EQ(lua_gettop(L), 0);
    }
};

TEST_F(CustomFunctionTest, CustomConstructor) {
    IntWrapper wrapper;
    wrapper = runWithResult<IntWrapper>(R"--(
        return IntWrapper:new()
    )--");
    EXPECT_EQ(wrapper.get(), 7);
    wrapper = runWithResult<IntWrapper>(R"--(
        return IntWrapper:new(13)
    )--");
    EXPECT_EQ(wrapper.get(), 13);
}

TEST_F(CustomFunctionTest, CustomFunction) {
    auto value = runWithResult<int>(R"--(
        obj1 = IntWrapper:new(13)
        obj1Table = obj1:toTable()

        obj2 = IntWrapper:create(14)
        obj2Table = obj2.table

        return obj1Table["value"] + obj2Table["value"]
    )--");

    EXPECT_EQ(value, 27);
}

TEST_F(CustomFunctionTest, ObjCustomTable) {
    run(R"--(
        obj = IntWrapper:new(13)
        obj.customProperty = 40
        return obj
    )--");

    luabind::user_data::get_custom_table(L, -1);
    lua_getfield(L, -1, "customProperty");
    auto value = lua_tointeger(L, -1);
    EXPECT_EQ(value, 40);
    lua_pop(L, 2);

    lua_newtable(L);
    lua_pushinteger(L, 34);
    lua_setfield(L, -2, "otherCustomProperty");
    luabind::user_data::set_custom_table(L, -2);

    lua_getfield(L, -1, "otherCustomProperty");
    value = lua_tointeger(L, -1);
    EXPECT_EQ(value, 34);
    lua_pop(L, 1);
}
