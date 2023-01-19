#ifndef LUABIND_USER_DATA
#define LUABIND_USER_DATA

#include "base.hpp"
#include "type_storage.hpp"

#include <memory>
#include <utility>

namespace luabind {

enum class memory_lifetime { lua, cpp, shared };

class user_data {
public:
    Object* const object;
    type_info* const info;
    const memory_lifetime lifetime;

protected:
    user_data(Object* object, type_info* info, memory_lifetime lifetime)
        : object(object)
        , info(info)
        , lifetime(lifetime) {}

    template <typename T>
    user_data(lua_State* L, T* object, memory_lifetime lifetime)
        : user_data(object, type_storage::find_type_info(L, object), lifetime) {}

public:
    virtual ~user_data() {
        const_cast<Object*&>(object) = nullptr;
    };

    static user_data* from_lua(lua_State* L, int idx) {
        if (lua_islightuserdata(L, idx) == 1) {
            return nullptr;
        }
        return static_cast<user_data*>(lua_touserdata(L, idx));
    }

public:
    static void get_destructing_metatable(lua_State* L) {
        lua_newtable(L);
        int table_idx = lua_gettop(L);
        add_destructing_functions(L, table_idx);
    }

    static void add_destructing_functions(lua_State* L, int table_idx) {
        lua_pushliteral(L, "__gc");
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto ud = from_lua(L, -1);
            if (ud->object != nullptr) {
                ud->~user_data();
            }
            return 0;
        });
        lua_pushliteral(L, "delete");
        lua_pushvalue(L, -2);
        lua_rawset(L, table_idx);
        lua_rawset(L, table_idx);
    }
};

template <typename T>
class lua_user_data : user_data {
public:
    T data;

    template <typename... Args>
    lua_user_data(lua_State* L, Args&&... args)
        : user_data(nullptr, type_storage::find_type_info<T>(L), memory_lifetime::lua)
        , data(std::forward<Args>(args)...) {
        // do not pass &data in user_data ctor, because in case of virtual inheritance
        // cast to Object* is implicitly dynamic_cast and needs fully initialized data
        const_cast<Object*&>(object) = &data;
    }

    template <typename... Args>
    static int to_lua(lua_State* L, Args&&... args) {
        static_assert(std::is_constructible_v<T, Args...>);
        void* p = lua_newuserdatauv(L, sizeof(lua_user_data), 0);
        lua_user_data* ud = new (p) lua_user_data(L, std::forward<Args>(args)...);
        if (ud->info != nullptr) {
            ud->info->get_metatable(L);
        } else {
            get_destructing_metatable(L);
        }
        lua_setmetatable(L, -2);
        return 1;
    }
};

template <typename T>
struct cpp_user_data : user_data {
    T* data;

    cpp_user_data(lua_State* L, T* v)
        : user_data(L, v, memory_lifetime::cpp)
        , data(v) {}

    static int to_lua(lua_State* L, T* v) {
        void* p = lua_newuserdatauv(L, sizeof(cpp_user_data), 0);
        cpp_user_data* ud = new (p) cpp_user_data(L, v);
        if (ud->info != nullptr) {
            ud->info->get_metatable(L);
        } else {
            get_destructing_metatable(L);
        }
        lua_setmetatable(L, -2);
        return 1;
    }
};

struct shared_user_data : user_data {
    std::shared_ptr<Object> data;

    template <typename T>
    shared_user_data(lua_State* L, std::shared_ptr<T> v)
        : user_data(L, v.get(), memory_lifetime::shared)
        , data(std::move(v)) {}

    template <typename T>
    static int to_lua(lua_State* L, std::shared_ptr<T> v) {
        void* p = lua_newuserdatauv(L, sizeof(shared_user_data), 0);
        shared_user_data* ud = new (p) shared_user_data(L, std::move(v));
        if (ud->info != nullptr) {
            ud->info->get_metatable(L);
        } else {
            get_destructing_metatable(L);
        }
        lua_setmetatable(L, -2);
        return 1;
    }
};

} // namespace luabind

#endif // LUABIND_USER_DATA
