#ifndef LUABIND_MIRROR_HPP
#define LUABIND_MIRROR_HPP

#include "lua.hpp"

#include "type_storage.hpp"
#include "user_data.hpp"

#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace luabind {

template <typename T>
struct value_mirror {
    using type = T;
    using raw_type = std::remove_cv_t<std::remove_reference_t<T>>;

    static_assert(std::is_class_v<raw_type>, "Only user types are supported by default implementation.");

    template <typename... Args>
    static int to_lua(lua_State* L, Args&&... args) {
        static_assert(std::is_constructible_v<T, Args...>);
        return lua_user_data<T>::to_lua(L, std::forward<Args>(args)...);
    }

    static const T& from_lua(lua_State* L, int idx) {
        return *(value_mirror<T*>::from_lua(L, idx));
    }
};

template <typename T>
struct value_mirror<T*> {
    using type = T*;
    using raw_type = std::remove_cv_t<T>;

    static int to_lua(lua_State* L, T* v) {
        cpp_user_data<T>::to_lua(L, v);
        return 1;
    }

    static T* from_lua(lua_State* L, int idx) {
        auto* ud = user_data::from_lua(L, idx);
        if (ud == nullptr) [[unlikely]] {
            reportError("Argument at %i has invalid type. Expecting user_data of type '%s', but got lua type '%s'",
                        idx,
                        type_storage::type_name<T>(L).data(),
                        lua_typename(L, lua_type(L, idx)));
        }
        auto p = dynamic_cast<T*>(ud->object);
        if (p == nullptr && ud->object != nullptr) [[unlikely]] {
            reportError("Argument at %i has invalid type. Expecting '%s' but got '%s'.",
                        idx,
                        type_storage::type_name<T>(L).data(),
                        ud->info->name.c_str());
        }
        return p;
    }
};

template <typename T>
struct value_mirror<T&> {
    static int to_lua(lua_State* L, T& v) {
        return value_mirror<T*>::to_lua(L, &v);
    }

    static T& from_lua(lua_State* L, int idx) {
        return *value_mirror<T*>::from_lua(L, idx);
    }
};

template <typename T>
struct value_mirror<std::shared_ptr<T>> {
    using type = std::shared_ptr<T>;

    static int to_lua(lua_State* L, type v) {
        return shared_user_data::to_lua(L, std::move(v));
    }

    static type from_lua(lua_State* L, int idx) {
        auto* ud = user_data::from_lua(L, idx);
        if (ud == nullptr) [[unlikely]] {
            reportError("Argument at %i has invalid type. Expecting user_data of type '%s', but got lua type '%s'",
                        idx,
                        type_storage::type_name<T>(L).data(),
                        lua_typename(L, lua_type(L, idx)));
        }
        auto sud = dynamic_cast<shared_user_data*>(ud);
        if (sud == nullptr) [[unlikely]] {
            reportError("Argument at %i is not a shared_ptr.", idx);
        }
        auto r = std::dynamic_pointer_cast<T>(sud->data);
        if (!r && sud->data) [[unlikely]] {
            reportError("Argument at %i has invalid type. Expecing '%s' but got '%s'.",
                        idx,
                        type_storage::type_name<T>(L).data(),
                        ud->info->name.c_str());
        }
        return r;
    }
};

template <typename T>
struct value_mirror<const std::shared_ptr<T>&> : value_mirror<std::shared_ptr<T>> {};

template <typename T>
struct value_mirror<std::shared_ptr<T>&> {};

template <typename T>
struct value_mirror<std::shared_ptr<T>&&> {};

template <>
struct value_mirror<bool> {
    using type = bool;

    static int to_lua(lua_State* L, bool v) {
        lua_pushboolean(L, static_cast<int>(v));
        return 1;
    }

    static bool from_lua(lua_State* L, int idx) {
        int isb = lua_isboolean(L, idx);
        if (isb != 1) [[unlikely]] {
            reportError("Argument at %i has invalid type. Expecting 'boolean', but got '%s'.",
                        idx,
                        lua_typename(L, lua_type(L, idx)));
        }
        int r = lua_toboolean(L, idx);
        return static_cast<bool>(r);
    }
};

template <typename T>
struct number_mirror {
    using raw_type = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(std::is_arithmetic_v<raw_type>, "This helper works only for builtin types.");

    static int to_lua(lua_State* L, raw_type v) {
        if constexpr (std::is_integral_v<raw_type>) {
            lua_pushinteger(L, static_cast<lua_Integer>(v));
            return 1;
        } else {
            lua_pushnumber(L, static_cast<lua_Number>(v));
        }
        return 1;
    }

    static raw_type from_lua(lua_State* L, int idx) {
        if constexpr (std::is_integral_v<raw_type>) {
            if (0 == lua_isinteger(L, idx)) {
                reportError("Argument at %i has invalid type. Expecting 'integer', but got '%s'.",
                            idx,
                            lua_typename(L, lua_type(L, idx)));
            }
            return static_cast<raw_type>(lua_tointeger(L, idx));
        } else {
            if (lua_type(L, idx) != LUA_TNUMBER) {
                reportError("Argument at %i has invalid type. Expecting 'number', but got '%s'.",
                            idx,
                            lua_typename(L, lua_type(L, idx)));
            }
            return static_cast<raw_type>(lua_tonumber(L, idx));
        }
    }
};

template <>
struct value_mirror<short> : number_mirror<short> {};
template <>
struct value_mirror<unsigned short> : number_mirror<unsigned short> {};
template <>
struct value_mirror<int> : number_mirror<int> {};
template <>
struct value_mirror<unsigned int> : number_mirror<unsigned int> {};
template <>
struct value_mirror<long> : number_mirror<long> {};
template <>
struct value_mirror<unsigned long> : number_mirror<unsigned long> {};

template <>
struct value_mirror<long long> : number_mirror<long long> {};
template <>
struct value_mirror<unsigned long long> : number_mirror<unsigned long long> {};

template <>
struct value_mirror<float> : number_mirror<float> {};
template <>
struct value_mirror<double> : number_mirror<double> {};

template <>
struct value_mirror<std::string_view> {
    static int to_lua(lua_State* L, const std::string_view v) {
        lua_pushlstring(L, v.data(), v.size());
        return 1;
    }

    static std::string_view from_lua(lua_State* L, int idx) {
        if (lua_type(L, idx) != LUA_TSTRING) {
            reportError("Argument at %i has invalid type. Expecting 'string', but got '%s'.",
                        idx,
                        lua_typename(L, lua_type(L, idx)));
        }
        size_t len;
        const char* lv = lua_tolstring(L, idx, &len);
        return std::string_view(lv, len);
    }
};

template <>
struct value_mirror<const std::string_view> : value_mirror<std::string_view> {};

template <>
struct value_mirror<const std::string_view&> : value_mirror<std::string_view> {};

template <>
struct value_mirror<std::string_view*> {};

template <>
struct value_mirror<const std::string_view*> {};

template <>
struct value_mirror<std::string> {
    static int to_lua(lua_State* L, const std::string& v) {
        return value_mirror<std::string_view>::to_lua(L, v);
    }

    static std::string from_lua(lua_State* L, int idx) {
        return std::string {value_mirror<std::string_view>::from_lua(L, idx)};
    }
};

template <>
struct value_mirror<const std::string> : value_mirror<std::string> {};

template <>
struct value_mirror<const std::string&> : value_mirror<std::string> {};

template <>
struct value_mirror<std::string*> {};

template <>
struct value_mirror<const std::string*> {};

template <typename T, typename Y>
struct value_mirror<std::pair<T, Y>> {
    using type = std::pair<T, Y>;

    static int to_lua(lua_State* L, const type& v) {
        lua_createtable(L, 2, 0);
        int t = lua_gettop(L);
        value_mirror<T>::to_lua(L, v.first);
        lua_rawseti(L, t, 1);
        value_mirror<Y>::to_lua(L, v.second);
        lua_rawseti(L, t, 2);
        return 1;
    }

    static type from_lua(lua_State* L, int idx) {
        if (lua_istable(L, idx) == 0) {
            reportError("Provided argument at %i for the pair is not a table.", idx);
        }
        if (lua_rawlen(L, idx) != 2) {
            reportError("Provided table at %i for the pair value has invalid length.", idx);
        }
        lua_rawgeti(L, idx, 1);
        T f = value_mirror<T>::from_lua(L, -1);
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 2);
        Y s = value_mirror<Y>::from_lua(L, -1);
        lua_pop(L, 1);
        return {f, s};
    }
};

} // namespace luabind

#endif // LUABIND_MIRROR_HPP
