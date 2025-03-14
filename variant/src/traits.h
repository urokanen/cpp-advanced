#pragma once

#include <cstddef>
#include <type_traits>

template <typename... Types>
class variant;

struct indexed_visit;

template <typename T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <std::size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <std::size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

class bad_variant_access : public std::exception {
public:
  const char* what() const noexcept override {
    return "bad variant access";
  }
};

inline constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

template <std::size_t I, typename Variant>
struct variant_alternative;

template <typename First, typename... Rest>
struct variant_alternative<0, variant<First, Rest...>> {
  using type = First;
};

template <std::size_t I, typename First, typename... Rest>
struct variant_alternative<I, variant<First, Rest...>> {
  using type = typename variant_alternative<I - 1, variant<Rest...>>::type;
};

template <std::size_t I, typename T>
struct variant_alternative<I, const T> {
  using type = const typename variant_alternative<I, T>::type;
};

template <std::size_t I, typename T>
struct variant_alternative<I, volatile T> {
  using type = volatile typename variant_alternative<I, T>::type;
};

template <std::size_t I, typename T>
struct variant_alternative<I, const volatile T> {
  using type = const volatile typename variant_alternative<I, T>::type;
};

template <std::size_t I, typename Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

template <typename T>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename Variant>
struct variant_size<const Variant> : variant_size<Variant> {};

template <typename Variant>
struct variant_size<volatile Variant> : variant_size<Variant> {};

template <typename Variant>
struct variant_size<const volatile Variant> : variant_size<Variant> {};

template <typename Variant>
inline constexpr std::size_t variant_size_v = variant_size<Variant>::value;

namespace impl {
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

template <std::size_t I, typename...>
struct get_type_by_index;

template <typename First, typename... Rest>
struct get_type_by_index<0, First, Rest...> {
  using type = First;
};

template <std::size_t I, typename First, typename... Rest>
  requires (I > 0)
struct get_type_by_index<I, First, Rest...> : get_type_by_index<I - 1, Rest...> {};

template <std::size_t I, typename... Types>
using get_type_by_index_t = typename get_type_by_index<I, Types...>::type;

template <typename T>
using arr = T[1];

template <typename A, typename T, typename = void>
struct can_initialize_array : std::false_type {};

template <typename A, typename T>
struct can_initialize_array<A, T, std::void_t<decltype(arr<A>{std::declval<T>()})>> : std::true_type {};

template <typename A, typename T>
constexpr bool can_initialize_array_v = can_initialize_array<A, T>::value;

template <std::size_t N, typename T, typename A>
struct single_overload {
  std::type_identity<A> operator()(A)
    requires (can_initialize_array_v<A, T>);
};

template <typename Seq, typename... Args>
struct overloads;

template <std::size_t... Idx, typename T, typename... Args>
struct overloads<std::integer_sequence<std::size_t, Idx...>, T, Args...> : single_overload<Idx, T, Args>... {
  using single_overload<Idx, T, Args>::operator()...;
};

template <typename T, typename... Types>
using chose_type =
    typename decltype(overloads<std::make_index_sequence<sizeof...(Types)>, T, Types...>{}(std::declval<T>()))::type;

template <typename T>
struct is_in_place {
  static constexpr bool value = false;
};

template <typename T>
struct is_in_place<in_place_type_t<T>> {
  static constexpr bool value = true;
};

template <std::size_t I>
struct is_in_place<in_place_index_t<I>> {
  static constexpr bool value = true;
};

template <typename T>
inline constexpr bool is_in_place_t = is_in_place<T>::value;

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

template <typename... Types>
struct traits {
  static constexpr bool trivially_copy_constr = (std::is_trivially_copy_constructible_v<Types> && ...);
  static constexpr bool trivially_move_constr = (std::is_trivially_move_constructible_v<Types> && ...);
  static constexpr bool trivially_destruct = (std::is_trivially_destructible_v<Types> && ...);

  static constexpr bool copy_constr = (std::is_copy_constructible_v<Types> && ...);
  static constexpr bool move_constr = (std::is_move_constructible_v<Types> && ...);
  static constexpr bool copy_assign = (std::is_copy_assignable_v<Types> && ...);
  static constexpr bool move_assign = (std::is_move_assignable_v<Types> && ...);

  static constexpr bool nothrow_copy_constr = (std::is_nothrow_copy_constructible_v<Types> && ...);
  static constexpr bool nothrow_move_constr = (std::is_nothrow_move_constructible_v<Types> && ...);
  static constexpr bool nothrow_move_assign = (std::is_nothrow_move_assignable_v<Types> && ...);
  static constexpr bool nothrow_swappable = (std::is_nothrow_swappable_v<Types> && ...);
  static constexpr bool swappable = (std::is_swappable_v<Types> && ...);

  static constexpr bool trivially_copy_assign =
      trivially_destruct && trivially_copy_constr && (std::is_trivially_copy_assignable_v<Types> && ...);
  static constexpr bool trivially_move_assign =
      trivially_destruct && trivially_move_constr && (std::is_trivially_move_assignable_v<Types> && ...);
};
} // namespace impl
