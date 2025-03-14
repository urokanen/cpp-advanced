#pragma once

#include <compare>
#include <memory>

struct nullopt_t {
  explicit constexpr nullopt_t(int) {}
};

inline constexpr nullopt_t nullopt{0};

struct in_place_t {};

inline constexpr in_place_t in_place;

namespace optional_impl {
enum class TInfo {
  DELETED,
  TRIVIAL,
  BASIC,
};

template <bool Trivial, bool Deleted>
constexpr TInfo chose_option() {
  if constexpr (Deleted) {
    return TInfo::DELETED;
  }
  if constexpr (Trivial) {
    return TInfo::TRIVIAL;
  }
  return TInfo::BASIC;
}

template <typename T, bool B = std::is_trivially_destructible_v<T>>
class optional_base;

template <typename T, typename... E>
constexpr void construct(optional_base<T>& base, E&&... other) {
  std::construct_at(std::addressof(base.object), std::forward<E>(other)...);
  base.is_initialized = true;
}

template <typename T>
class optional_base<T, true> {
public:
  constexpr optional_base() noexcept {}

  constexpr void clear() noexcept {
    if (is_initialized) {
      std::destroy_at(std::addressof(object));
      is_initialized = false;
    }
  }

  bool is_initialized = false;

  union {
    T object;
  };
};

template <typename T>
class optional_base<T, false> {
public:
  constexpr optional_base() noexcept {}

  constexpr ~optional_base() noexcept {
    clear();
  }

  constexpr void clear() noexcept(std::is_nothrow_destructible_v<T>) {
    if (is_initialized) {
      std::destroy_at(std::addressof(object));
      is_initialized = false;
    }
  }

  bool is_initialized = false;

  union {
    T object;
  };
};

template <
    typename T,
    TInfo State = chose_option<std::is_trivially_copy_constructible_v<T>, !std::is_copy_constructible_v<T>>()>
class optional_copy_ctor;

template <typename T>
class optional_copy_ctor<T, TInfo::TRIVIAL> : public optional_base<T> {
public:
  using optional_base<T>::optional_base;
};

template <typename T>
class optional_copy_ctor<T, TInfo::DELETED> : public optional_base<T> {
public:
  using optional_base<T>::optional_base;
  constexpr optional_copy_ctor(optional_copy_ctor&& other) = default;
  constexpr optional_copy_ctor& operator=(optional_copy_ctor&& other) = default;
  constexpr optional_copy_ctor& operator=(const optional_copy_ctor& other) = default;
  constexpr optional_copy_ctor(const optional_copy_ctor& other) = delete;
};

template <typename T>
class optional_copy_ctor<T, TInfo::BASIC> : public optional_base<T> {
public:
  using optional_base<T>::optional_base;
  constexpr optional_copy_ctor(optional_copy_ctor&& other) = default;
  constexpr optional_copy_ctor& operator=(optional_copy_ctor&& other) = default;
  constexpr optional_copy_ctor& operator=(const optional_copy_ctor& other) = default;

  constexpr optional_copy_ctor(const optional_copy_ctor& other) noexcept(std::is_nothrow_constructible_v<T, const T&>) {
    if (other.is_initialized) {
      construct(*this, other.object);
    }
  }
};

template <
    typename T,
    TInfo State = chose_option<std::is_trivially_move_constructible_v<T>, !std::is_move_constructible_v<T>>()>
class optional_move_ctor;

template <typename T>
class optional_move_ctor<T, TInfo::TRIVIAL> : public optional_copy_ctor<T> {
public:
  using optional_copy_ctor<T>::optional_copy_ctor;
};

template <typename T>
class optional_move_ctor<T, TInfo::DELETED> : public optional_copy_ctor<T> {
public:
  using optional_copy_ctor<T>::optional_copy_ctor;
  constexpr optional_move_ctor(const optional_move_ctor& other) = default;
  constexpr optional_move_ctor& operator=(optional_move_ctor&& other) = default;
  constexpr optional_move_ctor& operator=(const optional_move_ctor& other) = default;
  constexpr optional_move_ctor(optional_move_ctor&& other) = delete;
};

template <typename T>
class optional_move_ctor<T, TInfo::BASIC> : public optional_copy_ctor<T> {
public:
  using optional_copy_ctor<T>::optional_copy_ctor;
  constexpr optional_move_ctor(const optional_move_ctor& other) = default;
  constexpr optional_move_ctor& operator=(optional_move_ctor&& other) = default;
  constexpr optional_move_ctor& operator=(const optional_move_ctor& other) = default;

  constexpr optional_move_ctor(optional_move_ctor&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (other.is_initialized) {
      construct(*this, std::move(other.object));
    }
  }
};

template <
    typename T,
    TInfo State = chose_option<
        std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T> &&
            std::is_trivially_destructible_v<T>,
        !std::is_copy_constructible_v<T> || !std::is_copy_assignable_v<T>>()>
class optional_copy_assignment;

template <typename T>
class optional_copy_assignment<T, TInfo::TRIVIAL> : public optional_move_ctor<T> {
public:
  using optional_move_ctor<T>::optional_move_ctor;
};

template <typename T>
class optional_copy_assignment<T, TInfo::DELETED> : public optional_move_ctor<T> {
public:
  using optional_move_ctor<T>::optional_move_ctor;
  constexpr optional_copy_assignment(const optional_copy_assignment& other) = default;
  constexpr optional_copy_assignment(optional_copy_assignment&& other) = default;
  constexpr optional_copy_assignment& operator=(optional_copy_assignment&& other) = default;
  constexpr optional_copy_assignment& operator=(const optional_copy_assignment& other) = delete;
};

template <typename T>
class optional_copy_assignment<T, TInfo::BASIC> : public optional_move_ctor<T> {
public:
  using optional_move_ctor<T>::optional_move_ctor;
  constexpr optional_copy_assignment(const optional_copy_assignment& other) = default;
  constexpr optional_copy_assignment(optional_copy_assignment&& other) = default;
  constexpr optional_copy_assignment& operator=(optional_copy_assignment&& other) = default;

  constexpr optional_copy_assignment& operator=(const optional_copy_assignment& other
  ) noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
    if (this->is_initialized) {
      if (other.is_initialized) {
        this->object = other.object;
      } else {
        this->clear();
      }
    } else if (other.is_initialized) {
      construct(*this, other.object);
    }
    return *this;
  }
};

template <
    typename T,
    TInfo State = chose_option<
        std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> &&
            std::is_trivially_destructible_v<T>,
        !std::is_move_constructible_v<T> || !std::is_move_assignable_v<T>>()>
class optional_move_assignment;

template <typename T>
class optional_move_assignment<T, TInfo::TRIVIAL> : public optional_copy_assignment<T> {
public:
  using optional_copy_assignment<T>::optional_copy_assignment;
};

template <typename T>
class optional_move_assignment<T, TInfo::DELETED> : public optional_copy_assignment<T> {
public:
  using optional_copy_assignment<T>::optional_copy_assignment;
  constexpr optional_move_assignment(const optional_move_assignment& other) = default;
  constexpr optional_move_assignment(optional_move_assignment&& other) = default;
  constexpr optional_move_assignment& operator=(const optional_move_assignment& other) = default;
  constexpr optional_move_assignment& operator=(optional_move_assignment&& other) = delete;
};

template <typename T>
class optional_move_assignment<T, TInfo::BASIC> : public optional_copy_assignment<T> {
public:
  using optional_copy_assignment<T>::optional_copy_assignment;
  constexpr optional_move_assignment(const optional_move_assignment& other) = default;
  constexpr optional_move_assignment(optional_move_assignment&& other) = default;
  constexpr optional_move_assignment& operator=(const optional_move_assignment& other) = default;

  constexpr optional_move_assignment& operator=(optional_move_assignment&& other
  ) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
    if (this->is_initialized) {
      if (other.is_initialized) {
        this->object = std::move(other.object);
      } else {
        this->clear();
      }
    } else if (other.is_initialized) {
      construct(*this, std::move(other.object));
    }
    return *this;
  }
};
} // namespace optional_impl

template <typename T>
class optional : private optional_impl::optional_move_assignment<T> {
public:
  using value_type = T;
  using optional_impl::optional_move_assignment<T>::optional_move_assignment;

  constexpr optional(nullopt_t) noexcept
      : optional() {}

  template <
      typename U = T,
      std::enable_if_t<
          std::is_constructible_v<T, U&&> && !std::is_same_v<std::remove_cvref_t<U>, in_place_t> &&
              !std::is_same_v<std::remove_cvref_t<U>, optional>,
          bool> = true>
  constexpr explicit(!std::is_convertible_v<U&&, T>)
      optional(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
    construct(*this, std::forward<U>(value));
  }

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, bool> = true>
  explicit constexpr optional(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    construct(*this, std::forward<Args>(args)...);
  }

  constexpr optional& operator=(nullopt_t) noexcept {
    this->clear();
    return *this;
  }

  template <
      typename U = T,
      std::enable_if_t<
          !std::is_same_v<std::remove_cvref_t<U>, optional> && std::is_constructible_v<T, U> &&
              std::is_assignable_v<T&, U> && (!std::is_scalar_v<T> || !std::is_same_v<std::decay_t<U>, T>),
          bool> = true>
  constexpr optional& operator=(U&& value
  ) noexcept(std::is_nothrow_assignable_v<T&, U> && std::is_nothrow_constructible_v<T, U>) {
    if (this->is_initialized) {
      this->object = std::forward<U>(value);
    } else {
      construct(*this, std::forward<U>(value));
    }
    return *this;
  }

  constexpr bool has_value() const noexcept {
    return this->is_initialized;
  }

  constexpr explicit operator bool() const noexcept {
    return has_value();
  }

  constexpr T& operator*() & noexcept {
    return this->object;
  }

  constexpr const T& operator*() const& noexcept {
    return this->object;
  }

  constexpr T&& operator*() && noexcept {
    return std::move(this->object);
  }

  constexpr const T&& operator*() const&& noexcept {
    return std::move(this->object);
  }

  constexpr T* operator->() noexcept {
    return std::addressof(this->object);
  }

  constexpr const T* operator->() const noexcept {
    return std::addressof(this->object);
  }

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, bool> = true>
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    this->clear();
    construct(*this, std::forward<Args>(args)...);
    return this->object;
  }

  constexpr void reset() noexcept {
    this->clear();
  }
};

template <typename T, std::enable_if_t<std::is_move_constructible_v<T> && std::is_swappable_v<T>, bool> = true>
constexpr void swap(
    optional<T>& lhs,
    optional<T>& rhs
) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
  if (lhs.has_value() && rhs.has_value()) {
    using std::swap;
    swap(*lhs, *rhs);
  } else if (lhs.has_value()) {
    rhs.emplace(std::move(*lhs));
    lhs.reset();
  } else if (rhs.has_value()) {
    swap(rhs, lhs);
  }
}

template <typename T, std::enable_if_t<!std::is_move_constructible_v<T> || !std::is_swappable_v<T>, bool> = true>
constexpr void swap(optional<T>& lhs, optional<T>& rhs) = delete;

template <typename T>
constexpr bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
  return lhs.has_value() == rhs.has_value() && (!lhs.has_value() || (*lhs == *rhs));
}

template <typename T>
constexpr bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
  return lhs.has_value() != rhs.has_value() || (lhs.has_value() && (*lhs != *rhs));
}

template <typename T>
constexpr bool operator<(const optional<T>& lhs, const optional<T>& rhs) {
  return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}

template <typename T>
constexpr bool operator<=(const optional<T>& lhs, const optional<T>& rhs) {
  return !lhs.has_value() || (rhs.has_value() && *lhs <= *rhs);
}

template <typename T>
constexpr bool operator>(const optional<T>& lhs, const optional<T>& rhs) {
  return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}

template <typename T>
constexpr bool operator>=(const optional<T>& lhs, const optional<T>& rhs) {
  return !rhs.has_value() || (lhs.has_value() && *lhs >= *rhs);
}

template <class T>
constexpr std::compare_three_way_result_t<T> operator<=>(const optional<T>& lhs, const optional<T>& rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return lhs.has_value() <=> rhs.has_value();
  }
  return lhs.has_value() ? (*lhs <=> *rhs) : std::strong_ordering::equal;
}

template <typename T>
optional(T) -> optional<T>;
