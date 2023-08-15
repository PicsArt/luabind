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
    class_(lua_State* L, const std::string_view name)
        : _L(L) {
        _info = type_storage::add_type_info<Type, Bases...>(L, std::string {name}, index_impl, new_index_impl);
        _info->get_metatable(L);
        int mt_idx = lua_gettop(L);

        if constexpr (std::is_default_constructible_v<Type>) {
            lua_pushliteral(L, "new");
            lua_CFunction default_ctor = &ctor_wrapper<Type>::invoke;
            lua_pushcfunction(L, default_ctor);
            lua_rawset(L, mt_idx);
        }

        // one __index to rule them all and in lua bind them
        lua_pushliteral(L, "__index");
        lua_pushcfunction(L, &index_);
        lua_rawset(L, mt_idx);

        lua_pushliteral(L, "__newindex");
        lua_pushcfunction(L, &new_index);
        lua_rawset(L, mt_idx);

        user_data::add_destructing_functions(L, mt_idx);
        function("delete", user_data::destruct);
        lua_pop(L, 1); // pop metatable
    }

    template <typename... Args>
    class_& constructor(const std::string_view name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        return constructor(name, &ctor_wrapper<Type, Args...>::invoke);
    }

    template <typename... Args>
    class_& construct_shared(const std::string_view name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        return constructor(name, &shared_ctor_wrapper<Type, Args...>::invoke);
    }

    class_& constructor(const std::string_view name, lua_CFunction functor) {
        _info->get_metatable(_L);
        value_mirror<std::string_view>::to_lua(_L, name);
        lua_pushcfunction(_L, functor);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <auto func>
    class_& function(const std::string_view name) {
        return function(name, &function_wrapper<decltype(func), func>::invoke);
    }

    class_& function(const std::string_view name, lua_CFunction luaFunction) {
        _info->functions[std::string {name}] = luaFunction;
        return *this;
    }

    template <auto func>
    class_& class_function(const std::string_view name) {
        return class_function(name, &class_function_wrapper<decltype(func), func>::invoke);
    }

    class_& class_function(const std::string_view name, lua_CFunction luaFunction) {
        _info->get_metatable(_L);
        value_mirror<std::string_view>::to_lua(_L, name);
        lua_pushcfunction(_L, luaFunction);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <auto prop>
    class_& property_readonly(const std::string_view name) {
        return property(name, &property_wrapper<get, decltype(prop), prop>::invoke, nullptr);
    }

    template <auto prop>
    class_& property(const std::string_view name) {
        return property(name,
                        &property_wrapper<get, decltype(prop), prop>::invoke,
                        &property_wrapper<set, decltype(prop), prop>::invoke);
    }

    class_& property_readonly(const std::string_view name, lua_CFunction getter_function) {
        _info->properties.emplace(name, property_data(getter_function, nullptr));
        return *this;
    }

    class_& property(const std::string_view name, lua_CFunction getter_function, lua_CFunction setter_function) {
        _info->properties.emplace(name, property_data(getter_function, setter_function));
        return *this;
    }

    class_& array_access(lua_CFunction getter_function) {
        _info->array_access_getter = getter_function;
        return *this;
    }

    class_& array_access(lua_CFunction getter_function, lua_CFunction setter_function) {
        _info->array_access_getter = getter_function;
        _info->array_access_setter = setter_function;
        return *this;
    }

private:
    static int index_(lua_State* L) {
        int r = index_impl(L);
        if (r != 0) return r;
        // if there is no result from bound C++, look in the lua table bound to this object
        user_data::get_custom_table(L, 1); // custom table
        lua_pushvalue(L, 2); // key
        lua_rawget(L, -2);
        return 1;
    }

    static int index_impl(lua_State* L) {
        type_info* info = type_storage::find_type_info<Type>(L);

        auto key_type = lua_type(L, 2);
        if (key_type != LUA_TNUMBER && key_type != LUA_TSTRING) {
            luaL_error(L, "Key type should be number or string, '%s' is provided.", lua_typename(L, key_type));
        }

        if (key_type == LUA_TNUMBER && info->array_access_getter) {
            return info->array_access_getter(L);
        }

        if (key_type == LUA_TSTRING) {
            auto key = value_mirror<std::string_view>::from_lua(L, 2);
            auto p_it = info->properties.find(key);
            if (p_it != info->properties.end()) {
                if (p_it->second.getter) {
                    return p_it->second.getter(L);
                } else {
                    luaL_error(L, "Property named '%s' does not have getter.", key);
                }
            }
            auto f_it = info->functions.find(key);
            if (f_it != info->functions.end()) {
                lua_pushcfunction(L, f_it->second);
                return 1;
            }
        }
        // lua doesn't do recursive index lookup through metatables
        // if __index is bound to a C function, so we do it ourselves.
        for (type_info* base : info->bases) {
            int r = base->index(L);
            if (r != 0) {
                return r;
            }
        }

        return 0;
    }

    static int new_index(lua_State* L) {
        int r = new_index_impl(L);
        if (r != 0) return r;
        // if there is no result in C++ add new value to the lua table bound to this object
        user_data::get_custom_table(L, 1); // custom table
        lua_pushvalue(L, 2); // key
        lua_pushvalue(L, 3); // new value
        lua_rawset(L, -3);
        return 0;
    }

    static int new_index_impl(lua_State* L) {
        type_info* info = type_storage::find_type_info<Type>(L);

        auto key_type = lua_type(L, 2);
        if (key_type != LUA_TNUMBER && key_type != LUA_TSTRING) {
            luaL_error(L, "Key type should be number or string, '%s' is provided.", lua_typename(L, key_type));
        }

        if (key_type == LUA_TNUMBER && info->array_access_setter) {
            return info->array_access_setter(L);
        }

        if (key_type == LUA_TSTRING) {
            auto key = value_mirror<std::string_view>::from_lua(L, 2);
            auto p_it = info->properties.find(key);
            if (p_it != info->properties.end()) {
                if (p_it->second.setter) {
                    return p_it->second.setter(L);
                } else {
                    luaL_error(L, "Property named '%s' is read only.", key);
                }
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

inline void function(lua_State* L, const std::string_view name, lua_CFunction luaFunction) {
    lua_pushcfunction(L, luaFunction);
    lua_setglobal(L, name.data());
}

template <auto func>
void function(lua_State* L, const std::string_view name) {
    function(L, name, &function_wrapper<decltype(func), func>::invoke);
}

} // namespace luabind

#endif // LUABIND_BIND_HPP
