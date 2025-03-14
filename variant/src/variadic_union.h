#pragma once

#include "table.h"

namespace impl {
template <typename...>
union variadic_union {};

template <typename First, typename... Rest>
union variadic_union<First, Rest...> {
public:
  constexpr variadic_union() {}

  constexpr variadic_union(const variadic_union& other)
    requires (traits<First, Rest...>::trivially_copy_constr)
  = default;

  constexpr variadic_union(variadic_union&& other)
    requires (traits<First, Rest...>::trivially_move_constr)
  = default;

  template <typename... Args>
  constexpr explicit variadic_union(in_place_index_t<0>, Args&&... args)
      : first(std::forward<Args>(args)...) {}

  template <std::size_t I, typename... Args>
  constexpr explicit variadic_union(in_place_index_t<I>, Args&&... args)
      : rest(in_place_index<I - 1>, std::forward<Args>(args)...) {}

  constexpr ~variadic_union()
    requires (traits<First, Rest...>::trivially_destruct)
  = default;

  constexpr ~variadic_union() {}

  constexpr variadic_union& operator=(const variadic_union& other)
    requires (traits<First, Rest...>::trivially_copy_assign)
  = default;

  constexpr variadic_union& operator=(variadic_union&& other)
    requires (traits<First, Rest...>::trivially_move_assign)
  = default;

  template <std::size_t I>
  constexpr auto& get() & noexcept {
    if constexpr (I == 0) {
      return first;
    } else {
      return rest.template get<I - 1>();
    }
  }

  template <std::size_t I>
  constexpr auto&& get() && noexcept {
    if constexpr (I == 0) {
      return std::move(first);
    } else {
      return std::move(rest.template get<I - 1>());
    }
  }

  template <std::size_t I>
  constexpr const auto& get() const& noexcept {
    if constexpr (I == 0) {
      return first;
    } else {
      return rest.template get<I - 1>();
    }
  }

  template <std::size_t I>
  constexpr const auto&& get() const&& noexcept {
    if constexpr (I == 0) {
      return std::move(first);
    } else {
      return std::move(rest.template get<I - 1>());
    }
  }

private:
  template <typename... Types>
  friend class ::variant;

  First first;
  variadic_union<Rest...> rest;
};
} // namespace impl
