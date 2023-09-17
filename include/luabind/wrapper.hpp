#ifndef LUABIND_WRAPPER_HPP
#define LUABIND_WRAPPER_HPP

#include "traits.hpp"
#include "lua.hpp"
#include "mirror.hpp"

#include <exception>
#include <type_traits>

namespace luabind {

template <typename CRTP>
struct exception_safe_wrapper {
    static int safe_invoke(lua_State* L) {
        try {
            return CRTP::invoke(L);
        } catch (void*) {
            // lua throws lua_longjmp* if compiled with C++ exceptions when yielding or reporting error
            // rethrow to not interrupt lua logic flow in that case.
            // assuming that normal code would not throw pointer
            throw;
        } catch (const luabind::error& e) {
            lua_pushstring(L, e.what());
        } catch (const std::exception& e) {
            lua_pushstring(L, e.what());
        } catch (...) {
            lua_pushliteral(L, "Unknown error while trying to call C function from Lua.");
        }
        lua_error(L); // [[noreturn]]
        return 0;
    }
};

template <typename Type, typename... Args>
struct ctor_wrapper {
    static_assert(std::conjunction_v<valid_lua_arg<Args>...>);

    static int invoke(lua_State* L) {
        // 1st argument is the metatable
        return indexed_call_helper(L, index_sequence<2, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        // +1 first argument is the Type metatable
        if (num_args != sizeof...(Args) + 1) {
            reportError(
                "Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args - 1);
        }
        return lua_user_data<Type>::to_lua(L, value_mirror<Args>::from_lua(L, Indices)...);
    }
};

template <typename Type, typename... Args>
struct shared_ctor_wrapper {
    static_assert(std::conjunction_v<valid_lua_arg<Args>...>);

    static int invoke(lua_State* L) {
        // 1st argument is the metatable
        return indexed_call_helper(L, index_sequence<2, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        // +1 first argument is the Type metatable
        if (num_args != sizeof...(Args) + 1) {
            reportError(
                "Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args - 1);
        }
        return shared_user_data::to_lua(L, std::make_shared<Type>(value_mirror<Args>::from_lua(L, Indices)...));
    }
};

template <typename F, F f>
struct function_wrapper;

template <typename R, typename T, typename... Args, R (T::*func)(Args...)>
struct function_wrapper<R (T::*)(Args...), func> {
    static_assert(std::conjunction_v<valid_lua_arg<R>, valid_lua_arg<Args>...>);

    static int invoke(lua_State* L) {
        return indexed_call_helper(L, index_sequence<2, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        if (num_args != sizeof...(Args) + 1) {
            reportError(
                "Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args - 1);
        }
        T* self = value_mirror<T*>::from_lua(L, 1);
        if constexpr (std::is_same_v<R, void>) {
            (self->*func)(value_mirror<Args>::from_lua(L, Indices)...);
            return 0;
        } else {
            return value_mirror<R>::to_lua(L, (self->*func)(value_mirror<Args>::from_lua(L, Indices)...));
        }
    }
};

template <typename R, typename T, typename... Args, R (T::*func)(Args...) const>
struct function_wrapper<R (T::*)(Args...) const, func> {
    static int invoke(lua_State* L) {
        return indexed_call_helper(L, index_sequence<2, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        if (num_args != sizeof...(Args) + 1) {
            reportError(
                "Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args - 1);
        }
        const T* self = value_mirror<const T*>::from_lua(L, 1);
        if constexpr (std::is_same_v<R, void>) {
            (self->*func)(value_mirror<Args>::from_lua(L, Indices)...);
            return 0;
        } else {
            return value_mirror<R>::to_lua(L, (self->*func)(value_mirror<Args>::from_lua(L, Indices)...));
        }
    }
};

template <typename R, typename... Args, R (*func)(Args...)>
struct function_wrapper<R (*)(Args...), func> {
    static int invoke(lua_State* L) {
        return indexed_call_helper(L, index_sequence<1, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        if (num_args != sizeof...(Args)) {
            reportError("Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args);
        }
        if constexpr (std::is_same_v<R, void>) {
            (*func)(value_mirror<Args>::from_lua(L, Indices)...);
            return 0;
        } else {
            return value_mirror<R>::to_lua(L, (*func)(value_mirror<Args>::from_lua(L, Indices)...));
        }
    }
};

template <typename F, F f>
struct class_function_wrapper;

template <typename R, typename... Args, R (*func)(Args...)>
struct class_function_wrapper<R (*)(Args...), func> {
    static_assert(std::conjunction_v<valid_lua_arg<R>, valid_lua_arg<Args>...>);

    static int invoke(lua_State* L) {
        return indexed_call_helper(L, index_sequence<2, sizeof...(Args)> {});
    }

    template <size_t... Indices>
    static int indexed_call_helper(lua_State* L, std::index_sequence<Indices...>) {
        int num_args = lua_gettop(L);
        if (num_args != sizeof...(Args) + 1) {
            reportError(
                "Invalid number of arguments, should be %lu, but %i were given.", sizeof...(Args), num_args - 1);
        }
        if constexpr (std::is_same_v<R, void>) {
            (*func)(value_mirror<Args>::from_lua(L, Indices)...);
            return 0;
        } else {
            return value_mirror<R>::to_lua(L, (*func)(value_mirror<Args>::from_lua(L, Indices)...));
        }
    }
};

template <lua_CFunction func>
struct lua_function : exception_safe_wrapper<lua_function<func>> {
    static int invoke(lua_State* L) {
        return (*func)(L);
    }
};

template <typename Tag, typename P, P prop>
struct property_wrapper;

// tag struct to distinguish between getter/setter(s)
struct get {};
struct set {};

template <typename R, typename T, R(T::*prop)>
struct property_wrapper<get, R(T::*), prop> {
    static int invoke(lua_State* L) {
        T* self = value_mirror<T*>::from_lua(L, 1);
        return value_mirror<R>::to_lua(L, self->*prop);
    }
};

template <typename R, typename T, R (T::*func)()>
struct property_wrapper<get, R (T::*)(), func> {
    static int invoke(lua_State* L) {
        T* self = value_mirror<T*>::from_lua(L, 1);
        return value_mirror<R>::to_lua(L, (self->*func)());
    }
};

template <typename R, typename T, R (T::*func)() const>
struct property_wrapper<get, R (T::*)() const, func> {
    static int invoke(lua_State* L) {
        const T* self = value_mirror<T*>::from_lua(L, 1);
        return value_mirror<R>::to_lua(L, (self->*func)());
    }
};

template <typename R, typename T, R(T::*prop)>
struct property_wrapper<set, R(T::*), prop> {
    static int invoke(lua_State* L) {
        T* self = value_mirror<T*>::from_lua(L, 1);
        self->*prop = value_mirror<R>::from_lua(L, 3);
        return 0;
    }
};

template <typename R, typename T, void (T::*func)(R)>
struct property_wrapper<set, void (T::*)(R), func> {
    static int invoke(lua_State* L) {
        T* self = value_mirror<T*>::from_lua(L, 1);
        (self->*func)(value_mirror<R>::from_lua(L, 3));
        return 0;
    }
};

} // namespace luabind

#endif // LUABIND_WRAPPER_HPP
