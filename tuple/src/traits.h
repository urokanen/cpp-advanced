#pragma once

#include <type_traits>
#include <utility>

template <typename... Types>
class tuple;

template <typename T>
struct tuple_size;

template <typename... Types>
struct tuple_size<tuple<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename T>
inline constexpr std::size_t tuple_size_v = tuple_size<T>::value;

template <std::size_t N, typename T>
struct tuple_element;

template <typename First, typename... Rest>
struct tuple_element<0, tuple<First, Rest...>> {
  using type = First;
};

template <std::size_t N, typename First, typename... Rest>
  requires (N > 0)
struct tuple_element<N, tuple<First, Rest...>> : tuple_element<N - 1, tuple<Rest...>> {};

template <std::size_t N, typename T>
using tuple_element_t = typename tuple_element<N, T>::type;

namespace impl {
template <typename... Types>
struct get_first;

template <typename First, typename... Rest>
struct get_first<First, Rest...> {
  using type = First;
};

template <typename... Types>
using get_first_t = typename get_first<Types...>::type;

template <typename... Types>
struct count;

template <typename T>
struct count<T> {
  static constexpr std::size_t value = 0;
};

template <typename T, typename... Types>
struct count<T, T, Types...> {
  static constexpr std::size_t value = count<T, Types...>::value + 1;
};

template <typename T, typename Y, typename... Types>
struct count<T, Y, Types...> {
  static constexpr std::size_t value = count<T, Types...>::value;
};

template <typename T, typename... Types>
inline constexpr bool only_once = (count<T, Types...>::value == 1);

template <typename T, typename...>
struct get_index_by_type {
  static constexpr std::size_t value = 0;
};

template <typename T, typename First, typename... Rest>
struct get_index_by_type<T, First, Rest...> {
  static constexpr std::size_t value = std::is_same_v<T, First> ? 0 : get_index_by_type<T, Rest...>::value + 1;
};

template <typename... Args>
inline constexpr std::size_t get_index_by_type_v = get_index_by_type<Args...>::value;

template <typename... Types>
struct traits {
  static constexpr bool trivially_copy_constr = (std::is_trivially_copy_constructible_v<Types> && ...);
  static constexpr bool trivially_move_constr = (std::is_trivially_move_constructible_v<Types> && ...);
  static constexpr bool trivially_destruct = (std::is_trivially_destructible_v<Types> && ...);
  static constexpr bool default_constr = (std::is_default_constructible_v<Types> && ...);
  static constexpr bool copy_constr = (std::is_copy_constructible_v<Types> && ...);
  static constexpr bool move_constr = (std::is_move_constructible_v<Types> && ...);
  static constexpr bool const_ref_convert = (std::is_convertible_v<const Types&, Types> && ...);
  static constexpr bool nothrow_swappable = (std::is_nothrow_swappable_v<Types> && ...);
  static constexpr bool swappable = (std::is_swappable_v<Types> && ...);
  static constexpr bool trivially_copy_assign =
      trivially_destruct && trivially_copy_constr && (std::is_trivially_copy_assignable_v<Types> && ...);
  static constexpr bool trivially_move_assign =
      trivially_destruct && trivially_move_constr && (std::is_trivially_move_assignable_v<Types> && ...);
};

template <std::size_t N, typename T, typename... Types>
struct check_impl;

template <std::size_t N, typename... UTypes, typename... Types>
  requires (N < sizeof...(UTypes))
struct check_impl<N, tuple<UTypes...>, Types...> {
  static constexpr bool value =
      std::is_constructible_v<
          tuple_element_t<N, tuple<Types...>>,
          decltype(get<N>(std::forward<tuple<UTypes...>>(std::declval<tuple<UTypes...>>())))> &&
      check_impl<N + 1, tuple<UTypes...>, Types...>::value;
};

template <std::size_t N, typename... UTypes, typename... Types>
  requires (N == sizeof...(UTypes))
struct check_impl<N, tuple<UTypes...>, Types...> {
  static constexpr bool value = true;
};

template <typename T, typename... Types>
static constexpr bool check = check_impl<0, T, Types...>::value;

template <typename... Tuples>
struct return_value;

template <>
struct return_value<> {
  using type = tuple<>;
};

template <typename... Types>
struct return_value<tuple<Types...>> {
  using type = tuple<Types...>;
};

template <typename... Types1, typename... Types2, typename... Rest>
struct return_value<tuple<Types1...>, tuple<Types2...>, Rest...> {
  using type = return_value<tuple<Types1..., Types2...>, Rest...>::type;
};
} // namespace impl
