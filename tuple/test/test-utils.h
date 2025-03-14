#pragma once

#include <cstddef>
#include <iostream>
#include <utility>

namespace tests::util {

struct move_counter {
  move_counter() = default;

  move_counter(move_counter&& other) noexcept
      : moves(other.moves + 1) {}

  move_counter& operator=(move_counter&&) = default;

  std::size_t get_moves() const noexcept {
    return moves;
  }

private:
  std::size_t moves = 0;
};

struct copy_counter {
  copy_counter() = default;

  copy_counter(const copy_counter& other)
      : copies(other.copies + 1) {}

  std::size_t get_copies() const noexcept {
    return copies;
  }

private:
  std::size_t copies = 0;
};

struct combined_counter {
  combined_counter() = default;

  combined_counter(const combined_counter& other) noexcept
      : copies(other.copies + 1)
      , moves(other.moves) {}

  combined_counter(combined_counter&& other) noexcept
      : copies(other.copies)
      , moves(other.moves + 1) {}

  friend bool operator==(const combined_counter& lhs, const combined_counter& rhs) noexcept {
    return lhs.copies == rhs.copies && lhs.moves == rhs.moves;
  }

private:
  std::size_t copies = 0;
  std::size_t moves = 0;
};

struct non_copyable {
  explicit non_copyable(int value)
      : value(value) {}

  non_copyable(non_copyable&&) = default;
  non_copyable& operator=(non_copyable&&) = default;

  int get_value() const {
    return value;
  }

private:
  non_copyable(const non_copyable&) = delete;
  non_copyable& operator=(const non_copyable&) = delete;

  int value;
};

struct non_trivial {
  non_trivial(const non_trivial&) {}

  non_trivial(non_trivial&&) {}

  non_trivial& operator=(const non_trivial&) {
    return *this;
  }

  non_trivial& operator=(non_trivial&&) {
    return *this;
  }

  ~non_trivial() {}
};

struct only_movable {
  only_movable()
      : value(0) {}

  only_movable(int x)
      : value(x) {}

  only_movable(const only_movable&) = delete;

  only_movable(only_movable&& other)
      : value(other.value) {
    other.value = 0;
  }

  only_movable& operator=(only_movable& other) = delete;

  only_movable& operator=(only_movable&& other) {
    value = other.value;
    other.value = 0;
    return *this;
  }

  int value;
};

template <auto VALUE>
struct constant {
  constexpr operator decltype(VALUE)() const {
    return VALUE;
  }
};

template <auto... VALUES, typename F>
constexpr void static_for(const F& f) {
  (f(constant<VALUES>{}), ...);
}

template <std::integral T, T... VALUES, typename F>
constexpr void static_for(const F& f, std::integer_sequence<T, VALUES...> = {}) {
  (f(constant<VALUES>{}), ...);
}

template <std::size_t N, typename F>
constexpr void static_for_n(const F& f) {
  static_for(f, std::make_index_sequence<N>{});
}

enum class ref_kind {
  LVALUE,
  CONST_LVALUE,
  RVALUE,
  CONST_RVALUE,
};

inline std::ostream& operator<<(std::ostream& out, ref_kind kind) {
  switch (kind) {
  case ref_kind::LVALUE:
    return out << "lvalue";
  case ref_kind::CONST_LVALUE:
    return out << "const lvalue";
  case ref_kind::RVALUE:
    return out << "rvalue";
  case ref_kind::CONST_RVALUE:
    return out << "const rvalue";
  }
  return out << "unknown";
}

template <ref_kind REF_KIND, typename T>
decltype(auto) as_ref(T& value) {
  if constexpr (REF_KIND == ref_kind::LVALUE) {
    return value;
  }
  if constexpr (REF_KIND == ref_kind::CONST_LVALUE) {
    return std::as_const(value);
  }
  if constexpr (REF_KIND == ref_kind::RVALUE) {
    return std::move(value);
  }
  if constexpr (REF_KIND == ref_kind::CONST_RVALUE) {
    return std::move(std::as_const(value));
  }
}

template <typename F>
constexpr void for_each_ref_kind(const F& f) {
  static_for<ref_kind::LVALUE, ref_kind::CONST_LVALUE, ref_kind::RVALUE, ref_kind::CONST_RVALUE>(f);
}

} // namespace tests::util
