#pragma once

#include "storage.h"

#include <cstddef>

template <typename... Types>
class tuple {
public:
  constexpr tuple()
    requires (impl::traits<Types...>::default_constr)
  = default;

  constexpr explicit(!impl::traits<Types...>::const_ref_convert) tuple(const Types&... args)
    requires (sizeof...(Types) >= 1 && impl::traits<Types...>::copy_constr)
      : storage(args...) {}

  template <typename... UTypes>
  constexpr explicit(!(std::is_convertible_v<UTypes, Types> && ...)) tuple(UTypes&&... args)
    requires (
        sizeof...(Types) == sizeof...(UTypes) && sizeof...(Types) >= 1 &&
        (std::is_constructible_v<Types, UTypes> && ...) &&
        (sizeof...(Types) != 1 || !std::is_same_v<std::remove_cvref_t<impl::get_first_t<UTypes...>>, tuple>)
    )
      : storage(std::forward<UTypes>(args)...) {}

  template <typename... UTypes>
  constexpr tuple(const tuple<UTypes...>& other)
    requires (
        sizeof...(Types) == sizeof...(UTypes) && impl::check<tuple<UTypes...>, Types...> &&
        (sizeof...(Types) != 1 || (!std::is_convertible_v<decltype(other), impl::get_first_t<Types...>> &&
                                   !std::is_constructible_v<impl::get_first_t<Types...>, decltype(other)> &&
                                   !std::is_same_v<impl::get_first_t<Types...>, impl::get_first_t<UTypes...>>) )
    )
      : storage(other.storage) {}

  template <typename... UTypes>
  constexpr explicit(false) tuple(tuple<UTypes...>&& other)
    requires (
        sizeof...(Types) == sizeof...(UTypes) && impl::check<tuple<UTypes...>, Types...> &&
        (sizeof...(Types) != 1 || (!std::is_convertible_v<decltype(other), impl::get_first_t<Types...>> &&
                                   !std::is_constructible_v<impl::get_first_t<Types...>, decltype(other)> &&
                                   !std::is_same_v<impl::get_first_t<Types...>, impl::get_first_t<UTypes...>>) )
    )
      : storage(std::move(other.storage)) {}

  constexpr tuple(const tuple& other)
    requires (impl::traits<Types...>::copy_constr)
  = default;

  constexpr tuple(tuple&& other)
    requires (impl::traits<Types...>::move_constr)
  = default;

  constexpr tuple& operator=(const tuple& other) = default;

  constexpr tuple& operator=(tuple&& other) = default;

  template <typename... UTypes>
  constexpr tuple& operator=(const tuple<UTypes...>& other)
    requires (sizeof...(Types) == sizeof...(UTypes) && (std::is_assignable_v<Types&, const UTypes&> && ...))
  {
    storage = other.storage;
    return *this;
  }

  template <typename... UTypes>
  constexpr tuple& operator=(tuple<UTypes...>&& other)
    requires (sizeof...(Types) == sizeof...(UTypes) && (std::is_assignable_v<Types&, UTypes> && ...))
  {
    storage = std::move(other.storage);
    return *this;
  }

  constexpr void swap(tuple& other) noexcept(impl::traits<Types...>::nothrow_swappable)
    requires (impl::traits<Types...>::swappable)
  {
    storage.swap(other.storage);
  }

private:
  template <std::size_t N, typename... Args>
  friend constexpr tuple_element_t<N, tuple<Args...>>& get(tuple<Args...>& t) noexcept;
  template <std::size_t N, typename... Args>
  friend constexpr tuple_element_t<N, tuple<Args...>>&& get(tuple<Args...>&& t) noexcept;
  template <std::size_t N, typename... Args>
  friend constexpr const tuple_element_t<N, tuple<Args...>>& get(const tuple<Args...>& t) noexcept;
  template <std::size_t N, typename... Args>
  friend constexpr const tuple_element_t<N, tuple<Args...>>&& get(const tuple<Args...>&& t) noexcept;
  template <typename... TTypes, typename... UTypes>
  friend constexpr auto operator<=>(const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs);
  template <typename... UTypes>
  friend class tuple;

  impl::storage<Types...> storage;
};

// deduction guide to aid CTAD
template <typename... Types>
tuple(Types...) -> tuple<Types...>;

template <class... Types>
constexpr tuple<std::unwrap_ref_decay_t<Types>...> make_tuple(Types&&... args) {
  return tuple<std::unwrap_ref_decay_t<Types>...>(std::forward<Types>(args)...);
}

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>& get(tuple<Types...>& t) noexcept {
  return t.storage.template get<N>();
}

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>&& get(tuple<Types...>&& t) noexcept {
  return std::move(t.storage.template get<N>());
}

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>& get(const tuple<Types...>& t) noexcept {
  return t.storage.template get<N>();
}

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>&& get(const tuple<Types...>&& t) noexcept {
  return std::move(t.storage.template get<N>());
}

template <typename T, typename... Types>
constexpr T& get(tuple<Types...>& t) noexcept
  requires (impl::only_once<T, Types...>)
{
  return get<impl::get_index_by_type_v<T, Types...>>(t);
}

template <typename T, typename... Types>
constexpr T&& get(tuple<Types...>&& t) noexcept
  requires (impl::only_once<T, Types...>)
{
  return std::move(get<impl::get_index_by_type_v<T, Types...>>(t));
}

template <typename T, typename... Types>
constexpr const T& get(const tuple<Types...>& t) noexcept
  requires (impl::only_once<T, Types...>)
{
  return get<impl::get_index_by_type_v<T, Types...>>(t);
}

template <typename T, typename... Types>
constexpr const T&& get(const tuple<Types...>&& t) noexcept
  requires (impl::only_once<T, Types...>)
{
  return std::move(get<impl::get_index_by_type_v<T, Types...>>(t));
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator==(const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs)
  requires (sizeof...(TTypes) == sizeof...(UTypes))
{
  return [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
    return ((get<Indices>(lhs) == get<Indices>(rhs)) && ...);
  }(std::make_index_sequence<sizeof...(TTypes)>{});
}

namespace impl {
template <typename T, typename Type, typename UType>
constexpr T tuple_cmp(const Type&, const UType&, std::index_sequence<>) {
  return T::equivalent;
}

template <typename T, typename Type, typename UType, size_t Idx0, size_t... Idxs>
constexpr T tuple_cmp(const Type& t, const UType& u, std::index_sequence<Idx0, Idxs...>) {
  auto res = std::compare_three_way{}(get<Idx0>(t), get<Idx0>(u));
  return std::is_neq(res) ? res : tuple_cmp<T>(t, u, std::index_sequence<Idxs...>());
}

template <typename Res, typename... Types>
struct tuple_cat_impl;

template <typename Res, typename First, typename... Rest>
struct tuple_cat_impl<Res, First, Rest...> {
  template <typename... Elements>
  static constexpr Res func(First&& first, Rest&&... rest, Elements&&... elems) {
    return [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
      return tuple_cat_impl<Res, Rest...>::func(
          std::forward<Rest>(rest)...,
          std::forward<Elements>(elems)...,
          get<Indices>(std::forward<First>(first))...
      );
    }(std::make_index_sequence<tuple_size_v<std::remove_cvref_t<First>>>{});
  }
};

template <typename Res>
struct tuple_cat_impl<Res> {
  template <typename... Elements>
  static constexpr Res func(Elements&&... elems) {
    return Res(std::forward<Elements>(elems)...);
  }
};
} // namespace impl

template <typename... TTypes, typename... UTypes>
constexpr auto operator<=>(const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs) {
  using T = std::common_comparison_category_t<std::compare_three_way_result_t<TTypes, UTypes>...>;
  return impl::tuple_cmp<T>(lhs, rhs, std::index_sequence_for<TTypes...>());
}

template <typename... Tuples>
constexpr impl::return_value<std::remove_cvref_t<Tuples>...>::type tuple_cat(Tuples&&... args) {
  using Res = impl::return_value<std::remove_cvref_t<Tuples>...>::type;
  return impl::tuple_cat_impl<Res, Tuples...>::template func(std::forward<Tuples>(args)...);
}

template <typename... Types>
constexpr void swap(tuple<Types...>& lhs, tuple<Types...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  requires (std::is_swappable_v<Types> && ...)
{
  lhs.swap(rhs);
}
