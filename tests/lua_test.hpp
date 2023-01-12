#pragma once

class LuaTest : public testing::Test {
protected:
    LuaTest() {
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    ~LuaTest() override {
        if (L != nullptr) {
            lua_close(L);
        }
    }

public:
    int run(const char* script) {
        int r = luaL_dostring(L, script);
        if (r != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            std::cerr << "Error: " << error << std::endl;
        }
        return r;
    }

    template <typename T>
    T runWithResult(const char* script) {
        int r = luaL_dostring(L, script);
        if (r != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            std::cerr << "Error: " << error << std::endl;
            EXPECT_EQ(r, LUA_OK);
            static T t {};
            return t;
        }
        EXPECT_EQ(lua_gettop(L), 1);
        T t = luabind::value_mirror<T>::from_lua(L, -1);
        lua_pop(L, -1);
        return t;
    }

protected:
    lua_State* L = nullptr;
};
