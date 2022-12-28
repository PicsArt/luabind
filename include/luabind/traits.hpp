#ifndef LUABIND_TRAITS_HPP
#define LUABIND_TRAITS_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

namespace luabind {

template <size_t Start, typename Seq>
struct index_sequence_offset;

template <size_t Start, size_t... Ints>
struct index_sequence_offset<Start, std::index_sequence<Ints...>> {
    using type = std::index_sequence<Start + Ints...>;
};

template <size_t Start, size_t Count>
using index_sequence = typename index_sequence_offset<Start, decltype(std::make_index_sequence<Count>())>::type;

template <typename From, typename To, typename = void>
struct can_static_cast : std::false_type {};

template <typename From, typename To>
struct can_static_cast<From, To, std::void_t<decltype(static_cast<To>(std::declval<From>()))>> : std::true_type {};

template <typename Base, typename Derived>
struct is_virtual_base_of
    : std::conjunction<std::is_base_of<Base, Derived>, std::negation<can_static_cast<Base*, Derived*>>> {};

template <typename F, F f>
struct function_result;

template <typename R, typename T, typename... Args, R (T::*func)(Args...)>
struct function_result<R (T::*)(Args...), func> {
    using type = R;
};

template <typename R, typename... Args, R (*func)(Args...)>
struct function_result<R (*)(Args...), func> {
    using type = R;
};

template <typename F, F f>
using function_result_t = typename function_result<F, f>::type;

template <typename T>
struct is_pointer_ref : public std::false_type {};

template <typename T>
struct is_pointer_ref<T*&> : public std::true_type {};

template <typename T>
inline constexpr bool is_pointer_ref_v = is_pointer_ref<T>::value;

// void f([const] int) // ok
// void f([const] int&) // not supported
// void f([const] int*) // not supported
// class T;
// void f([const] A) // ok
// void f([const] A&) // ok
// void f(A&&) // ok
// void f([const] A*) // ok
// void f([const] A*&) // not supported
template <typename T>
struct valid_lua_arg
    : std::bool_constant<!is_pointer_ref_v<T> &&
                         ((std::is_fundamental_v<T> && !std::is_pointer_v<T> && !std::is_reference_v<T>) ||
                          std::is_class_v<T>)> {};

} // namespace luabind

#endif // LUABIND_TRAITS_HPP
