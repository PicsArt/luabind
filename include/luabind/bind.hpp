#ifndef LUABIND_BIND_HPP
#define LUABIND_BIND_HPP

#include "base.hpp"
#include "exception.hpp"
#include "mirror.hpp"
#include "type_storage.hpp"
#include "traits.hpp"
#include "wrapper.hpp"

#include <iostream>
#include <map>
#include <tuple>
#include <type_traits>
#include <typeindex>

namespace luabind {

template <typename Type, typename... Bases>
class class_ {
    static_assert(std::is_base_of_v<Object, Type>, "Type should be descendant from luabind::Object.");

public:
    class_(lua_State* L, const char* name)
        : _L(L) {
        _info = type_storage::add_type_info<Type, Bases...>(L, name, index_impl, new_index_impl);
        _info->get_metatable(L);
        int table_idx = lua_gettop(L);

        if constexpr (std::is_default_constructible_v<Type>) {
            lua_pushliteral(L, "new");
            lua_CFunction default_ctor = &ctor_wrapper<Type>::invoke;
            lua_pushcfunction(L, default_ctor);
            lua_rawset(L, -3);
        }

        // one __index to rule them all and in lua bind them
        lua_pushliteral(L, "__index");
        lua_pushcfunction(L, &index_);
        lua_rawset(L, -3);

        lua_pushliteral(L, "__newindex");
        lua_pushcfunction(L, &new_index);
        lua_rawset(L, -3);

        user_data::add_destructing_functions(L, table_idx);
        lua_pop(L, 1); // pop metatable
    }

    template <typename... Args>
    class_& constructor(const char* name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        _info->get_metatable(_L);
        lua_pushstring(_L, name);
        lua_CFunction ctor = &ctor_wrapper<Type, Args...>::invoke;
        lua_pushcfunction(_L, ctor);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <typename... Args>
    class_& construct_shared(const char* name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        _info->get_metatable(_L);
        lua_pushstring(_L, name);
        lua_CFunction ctor = &shared_ctor_wrapper<Type, Args...>::invoke;
        lua_pushcfunction(_L, ctor);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <auto func>
    class_& function(const char* name) {
        lua_CFunction cwrapper = &function_wrapper<decltype(func), func>::invoke;
        _info->functions[name] = cwrapper;
        return *this;
    }

    class_& function(const char* name, lua_CFunction luaFunction) {
        _info->functions[name] = luaFunction;
        return *this;
    }

    // template <auto read_func, auto len_func, auto write_func = read_func>
    // class_& index() {
    //     _info->subscript_read = &function_wrapper<decltype(read_func), read_func>::invoke;
    //     if constexpr (write_func != read_func) {
    //         std::cout << "wohoo, writing subscript" << std::endl;
    //         _info->subscript_write = &function_wrapper<decltype(write_func), write_func>::invoke;
    //     }
    //     lua_CFunction len_function = &function_wrapper<decltype(len_func), len_func>::invoke;
    //     _info->get_metatable(_L);
    //     lua_pushliteral(_L, "__len");
    //     lua_pushcfunction(_L, len_function);
    //     lua_rawset(_L, -3);
    //     return *this;
    // }

    template <auto prop>
    class_& property_readonly(const char* name) {
        lua_CFunction getter_function = &property_wrapper<get, decltype(prop), prop>::invoke;
        _info->properties.emplace(name, property_data(getter_function, nullptr));
        return *this;
    }

    template <auto prop>
    class_& property(const char* name) {
        lua_CFunction getter_function = &property_wrapper<get, decltype(prop), prop>::invoke;
        lua_CFunction setter_function = &property_wrapper<set, decltype(prop), prop>::invoke;
        _info->properties.emplace(name, property_data(getter_function, setter_function));
        return *this;
    }

private:
    static int index_(lua_State* L) {
        int r = index_impl(L);
        if (r == 0) {
            const char* key = lua_tostring(L, 2);
            luaL_error(L, "object does not have function or property named: %s", key);
        }
        return r;
    }

    static int index_impl(lua_State* L) {
        type_info* info = type_storage::find_type_info<Type>(L);
        // if (lua_isinteger(L, 2) == 1) {
        //     if (info->subscript_read == nullptr) {
        //         lua_pushstring(L, "Indexing operation is not set");
        //         lua_error(L);
        //     }
        //     auto i = lua_tointeger(L, 2);
        //     lua_pop(L, 1);
        //     lua_pushinteger(L, i - 1);
        //     return info->subscript_read(L);
        // }
        const char* key = lua_tostring(L, 2);
        auto p_it = info->properties.find(std::string_view(key));
        if (p_it != info->properties.end()) {
            return p_it->second.getter(L);
        }
        auto f_it = info->functions.find(std::string_view(key));
        if (f_it != info->functions.end()) {
            lua_pushcfunction(L, f_it->second);
            return 1;
        }
        // lua doesn't do recursive index lookup through metatables
        // if __index is bound to a C function, so we do it ourselves.
        for (type_info* base : info->bases) {
            int r = base->index(L);
            if (r != 0) {
                return r;
            }
        }

        // Check if there is a function in metatable with the specified key
        info->get_metatable(L);
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            return 1;
        }

        return 0;
    }

    static int new_index(lua_State* L) {
        int r = new_index_impl(L);
        if (r == 0) {
            const char* key = lua_tostring(L, 2);
            luaL_error(L, "object does not have function or property named: %s", key);
        }
        return 0;
    }

    static int new_index_impl(lua_State* L) {
        type_info* info = type_storage::find_type_info<Type>(L);
        // TODO use lua_rotate to fix integral index i-=1;
        const char* key = lua_tostring(L, 2);
        auto p_it = info->properties.find(std::string_view(key));
        if (p_it != info->properties.end()) {
            if (p_it->second.setter) {
                p_it->second.setter(L);
                return 1;
            } else {
                luaL_error(L, "property named '%s' is read only", key);
            }
        }
        for (type_info* base : info->bases) {
            int r = base->new_index(L);
            if (r != 0) {
                return r;
            }
        }
        return 0;
    }

private:
    lua_State* _L;
    type_info* _info;
};

template <auto func>
void function(lua_State* L, const char* name) {
    lua_CFunction cwrapper = &function_wrapper<decltype(func), func>::invoke;
    lua_pushcfunction(L, cwrapper);
    lua_setglobal(L, name);
}

inline void function(lua_State* L, const char* name, lua_CFunction luaFunction) {
    lua_pushcfunction(L, luaFunction);
    lua_setglobal(L, name);
}

} // namespace luabind

#endif // LUABIND_BIND_HPP
