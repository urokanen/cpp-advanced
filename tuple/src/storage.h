#pragma once

#include "traits.h"

#include <compare>

namespace impl {

template <typename...>
class storage {
public:
  constexpr void swap([[maybe_unused]] storage& other) noexcept {}
};

template <typename First, typename... Rest>
class storage<First, Rest...> {
public:
  constexpr storage()
      : first()
      , rest() {}

  constexpr storage(const storage& other)
    requires (traits<First, Rest...>::trivially_copy_constr)
  = default;

  constexpr storage(storage&& other)
    requires (traits<First, Rest...>::trivially_move_constr)
  = default;

  template <typename... UTypes>
  constexpr storage(const storage<UTypes...>& other)
      : first(other.first)
      , rest(other.rest) {}

  template <typename... UTypes>
  constexpr storage(storage<UTypes...>&& other)
      : first(std::move(other.first))
      , rest(std::move(other.rest)) {}

  template <typename Arg, typename... Args>
  constexpr explicit storage(Arg&& arg, Args&&... args)
      : first(std::forward<Arg>(arg))
      , rest(std::forward<Args>(args)...) {}

  constexpr ~storage()
    requires (traits<First, Rest...>::trivially_destruct)
  = default;

  constexpr ~storage() {}

  constexpr storage& operator=(const storage& other)
    requires (traits<First, Rest...>::trivially_copy_assign)
  = default;

  constexpr storage& operator=(storage&& other)
    requires (traits<First, Rest...>::trivially_move_assign)
  = default;

  template <typename... UTypes>
  constexpr storage& operator=(const storage<UTypes...>& other) {
    first = other.first;
    rest = other.rest;
    return *this;
  }

  template <typename... UTypes>
  constexpr storage& operator=(storage<UTypes...>&& other) {
    first = (std::move(other.first));
    rest = (std::move(other.rest));
    return *this;
  }

  constexpr storage& operator=(const storage& other) {
    if (this != &other) {
      first = other.first;
      rest = other.rest;
    }
    return *this;
  }

  constexpr storage& operator=(storage&& other) {
    if (this != &other) {
      first = std::move(other.first);
      rest = std::move(other.rest);
    }
    return *this;
  }

  constexpr void swap(storage& other
  ) noexcept(std::is_nothrow_swappable_v<First> && (std::is_nothrow_swappable_v<Rest> && ...)) {
    using std::swap;
    swap(first, other.first);
    swap(rest, other.rest);
  }

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
  constexpr auto& get() const& noexcept {
    if constexpr (I == 0) {
      return first;
    } else {
      return rest.template get<I - 1>();
    }
  }

  template <std::size_t I>
  constexpr auto&& get() const&& noexcept {
    if constexpr (I == 0) {
      return std::move(first);
    } else {
      return std::move(rest.template get<I - 1>());
    }
  }

private:
  template <typename... UTypes>
  friend class storage;

  First first;
  storage<Rest...> rest;
};

template <typename... Types>
constexpr void swap(storage<Types...>& lhs, storage<Types...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  requires (std::is_swappable_v<Types> && ...)
{
  lhs.swap(rhs);
}

} // namespace impl
