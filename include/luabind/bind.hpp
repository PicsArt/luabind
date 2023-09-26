#ifndef LUABIND_BIND_HPP
#define LUABIND_BIND_HPP

#include "object.hpp"
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
            lua_CFunction default_ctor = ctor_wrapper<Type>::invoke;
            lua_pushcfunction(L, default_ctor);
            lua_rawset(L, mt_idx);
        }

        // one __index to rule them all and in lua bind them
        lua_pushliteral(L, "__index");
        lua_pushcfunction(L, lua_function<index_>::safe_invoke);
        lua_rawset(L, mt_idx);

        lua_pushliteral(L, "__newindex");
        lua_pushcfunction(L, lua_function<new_index>::safe_invoke);
        lua_rawset(L, mt_idx);

        user_data::add_destructing_functions(L, mt_idx);
        function<user_data::destruct>("delete");
        lua_pop(L, 1); // pop metatable
    }

    template <typename... Args>
    class_& constructor(const std::string_view name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        return constructor<ctor_wrapper<Type, Args...>::invoke>(name);
    }

    template <typename... Args>
    class_& construct_shared(const std::string_view name) {
        static_assert(std::is_constructible_v<Type, Args...>, "class should be constructible with given arguments");
        return constructor<shared_ctor_wrapper<Type, Args...>::invoke>(name);
    }

    template <lua_CFunction func>
    class_& constructor(const std::string_view name) {
        _info->get_metatable(_L);
        value_mirror<std::string_view>::to_lua(_L, name);
        lua_pushcfunction(_L, lua_function<func>::safe_invoke);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <auto func>
        requires(!std::is_same_v<decltype(func), lua_CFunction>)
    class_& function(const std::string_view name) {
        return function<function_wrapper<decltype(func), func>::invoke>(name);
    }

    template <lua_CFunction func>
    class_& function(const std::string_view name) {
        _info->functions[std::string {name}] = lua_function<func>::safe_invoke;
        return *this;
    }

    template <auto func>
        requires(!std::is_same_v<decltype(func), lua_CFunction>)
    class_& class_function(const std::string_view name) {
        return class_function<class_function_wrapper<decltype(func), func>::invoke>(name);
    }

    template <lua_CFunction func>
    class_& class_function(const std::string_view name) {
        _info->get_metatable(_L);
        value_mirror<std::string_view>::to_lua(_L, name);
        lua_pushcfunction(_L, lua_function<func>::safe_invoke);
        lua_rawset(_L, -3);
        lua_pop(_L, 1); // pop metatable
        return *this;
    }

    template <auto prop>
        requires(std::is_member_pointer_v<decltype(prop)>)
    class_& property_readonly(const std::string_view name) {
        return property_readonly<property_wrapper<get, decltype(prop), prop>::invoke>(name);
    }

    template <lua_CFunction func>
    class_& property_readonly(const std::string_view name) {
        _info->properties.emplace(name, property_data(lua_function<func>::safe_invoke, nullptr));
        return *this;
    }

    template <auto prop>
        requires(std::is_member_pointer_v<decltype(prop)>)
    class_& property(const std::string_view name) {
        if constexpr (std::is_member_object_pointer_v<decltype(prop)>) {
            return property<property_wrapper<get, decltype(prop), prop>::invoke,
                            property_wrapper<set, decltype(prop), prop>::invoke>(name);
        } else {
            return property_readonly<property_wrapper<get, decltype(prop), prop>::invoke>(name);
        }
    }

    template <auto getter, auto setter>
        requires(std::is_member_pointer_v<decltype(getter)> && std::is_member_pointer_v<decltype(setter)>)
    class_& property(const std::string_view name) {
        return property<property_wrapper<get, decltype(getter), getter>::invoke,
                        property_wrapper<set, decltype(setter), setter>::invoke>(name);
    }

    template <lua_CFunction getter, lua_CFunction setter>
    class_& property(const std::string_view name) {
        _info->properties.emplace(name,
                                  property_data(lua_function<getter>::safe_invoke, lua_function<setter>::safe_invoke));
        return *this;
    }

    template <auto getter>
        requires(!std::is_same_v<decltype(getter), lua_CFunction>)
    class_& array_access() {
        return array_access(function_wrapper<decltype(getter), getter>::invoke);
    }

    template <lua_CFunction getter>
    class_& array_access() {
        _info->array_access_getter = lua_function<getter>::safe_invoke;
        return *this;
    }

    template <auto getter, auto setter>
        requires(!std::is_same_v<decltype(getter), lua_CFunction> && !std::is_same_v<decltype(setter), lua_CFunction>)
    class_& array_access() {
        return array_access<function_wrapper<decltype(getter), getter>::invoke,
                            function_wrapper<decltype(setter), setter>::invoke>();
    }

    template <lua_CFunction getter, lua_CFunction setter>
    class_& array_access() {
        _info->array_access_getter = lua_function<getter>::safe_invoke;
        _info->array_access_setter = lua_function<setter>::safe_invoke;
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

        const bool is_integer = lua_isinteger(L, 2);
        const int key_type = lua_type(L, 2);
        if (!is_integer && key_type != LUA_TSTRING) {
            reportError("Key type should be integer or string, '%s' is provided.", lua_typename(L, key_type));
        }

        if (is_integer) {
            if (info->array_access_getter == nullptr) {
                reportError("Type '%s' does not provide array get access.", info->name.c_str());
            }
            return info->array_access_getter(L);
        }

        // key is string
        auto key = value_mirror<std::string_view>::from_lua(L, 2);
        auto p_it = info->properties.find(key);
        if (p_it != info->properties.end()) {
            if (p_it->second.getter == nullptr) {
                reportError("Property named '%s' does not have a getter.", key.data());
            }
            return p_it->second.getter(L);
        }
        auto f_it = info->functions.find(key);
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

        return 0;
    }

    static int new_index(lua_State* L) {
        int r = new_index_impl(L);
        if (r != 0) return 0;
        // if there is no result in C++ add new value to the lua table bound to this object
        user_data::get_custom_table(L, 1); // custom table
        lua_pushvalue(L, 2); // key
        lua_pushvalue(L, 3); // new value
        lua_rawset(L, -3);
        return 0;
    }

    static int new_index_impl(lua_State* L) {
        type_info* info = type_storage::find_type_info<Type>(L);

        const bool is_integer = lua_isinteger(L, 2);
        const int key_type = lua_type(L, 2);
        if (!is_integer && key_type != LUA_TSTRING) {
            reportError("Key type should be integer or string, '%s' is provided.", lua_typename(L, key_type));
        }
        if (is_integer) {
            if (info->array_access_setter == nullptr) {
                reportError("Type '%s' does not provide array set access.", info->name.c_str());
            }
            info->array_access_setter(L);
            return 1;
        }
        // key is a string
        auto key = value_mirror<std::string_view>::from_lua(L, 2);
        auto p_it = info->properties.find(key);
        if (p_it != info->properties.end()) {
            if (p_it->second.setter == nullptr) {
                reportError("Property '%s' is read only.", key.data());
            }
            p_it->second.setter(L);
            return 1;
        }

        // lua doesn't do recursive index lookup through metatables
        // if __new_index is bound to a C function, so we do it ourselves.
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

template <lua_CFunction func>
inline void function(lua_State* L, const std::string_view name) {
    lua_pushcfunction(L, lua_function<func>::safe_invoke);
    lua_setglobal(L, name.data());
}

template <auto func>
    requires(!std::is_same_v<decltype(func), lua_CFunction>)
void function(lua_State* L, const std::string_view name) {
    function<function_wrapper<decltype(func), func>::invoke>(L, name);
}

} // namespace luabind

#endif // LUABIND_BIND_HPP
