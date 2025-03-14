#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

class test_object {
public:
  test_object() = default;

  explicit test_object(int value)
      : a(value) {}

  test_object(const test_object&) = delete;

  test_object(test_object&& other) noexcept {
    std::swap(a, other.a);
  }

  test_object& operator=(const test_object&) = delete;

  test_object& operator=(test_object&& other) noexcept {
    std::swap(a, other.a);
    return *this;
  }

  friend bool operator<(const test_object& lhs, const test_object& rhs) {
    return lhs.a < rhs.a;
  }

  friend bool operator==(const test_object& lhs, const test_object& rhs) {
    return lhs.a == rhs.a;
  }

  int a = 0;
};

class vector_compare {
public:
  using vec = std::pair<int, int>;

  enum distance_type {
    euclidean,
    manhattan
  };

  explicit vector_compare(distance_type p = euclidean)
      : type(p) {}

  bool operator()(vec lhs, vec rhs) const {
    if (type == euclidean) {
      return euc(lhs) < euc(rhs);
    } else {
      return man(lhs) < man(rhs);
    }
  }

private:
  static double euc(vec x) {
    return sqrt(x.first * x.first + x.second * x.second);
  }

  static double man(vec x) {
    return abs(x.first) + abs(x.second);
  }

  distance_type type;
};

class non_default_constructible {
public:
  non_default_constructible() = delete;

  explicit non_default_constructible(int value)
      : a(value) {}

  non_default_constructible(const non_default_constructible&) = default;
  non_default_constructible& operator=(const non_default_constructible&) = default;

  friend bool operator<(const non_default_constructible& lhs, const non_default_constructible& rhs) {
    return lhs.a < rhs.a;
  }

  friend bool operator==(const non_default_constructible& lhs, const non_default_constructible& rhs) {
    return lhs.a == rhs.a;
  }

private:
  int a;
};

class non_copy_assignable {
public:
  non_copy_assignable()
      : a(0) {}

  explicit non_copy_assignable(int value)
      : a(value) {}

  non_copy_assignable(const non_copy_assignable&) = default;

  non_copy_assignable& operator=(const non_copy_assignable&) = delete;

  friend bool operator<(const non_copy_assignable& lhs, const non_copy_assignable& rhs) {
    return lhs.a < rhs.a;
  }

  friend bool operator==(const non_copy_assignable& lhs, const non_copy_assignable& rhs) {
    return lhs.a == rhs.a;
  }

private:
  int a;
};

class address_checking_object {
private:
  static std::unordered_set<const address_checking_object*> addresses;

  void add_instance() const;
  void remove_instance(bool nothrow = false) const;
  void assert_exists() const;

  int value;

  static size_t copy_throw_countdown;
  static void process_copying();

public:
  static void expect_no_instances();

  static void set_copy_throw_countdown(size_t new_countdown);

  operator int() const;

  address_checking_object();
  address_checking_object(int value);

  address_checking_object(const address_checking_object& other);
  address_checking_object& operator=(const address_checking_object& other);

  ~address_checking_object() noexcept(false);
};

class state_comparator {
public:
  explicit state_comparator(bool flag = false)
      : is_inverted(flag) {}

  bool operator()(int lhs, int rhs) const {
    if (is_inverted) {
      return rhs < lhs;
    } else {
      return lhs < rhs;
    }
  }

private:
  bool is_inverted;
};

class non_copyable_comparator : public std::less<> {
public:
  non_copyable_comparator() = default;

  non_copyable_comparator(const non_copyable_comparator&) = delete;
  non_copyable_comparator(non_copyable_comparator&&) = default;

  non_copyable_comparator& operator=(const non_copyable_comparator&) = delete;
  non_copyable_comparator& operator=(non_copyable_comparator&&) = default;

  ~non_copyable_comparator() = default;
};

class non_default_constructible_comparator : public std::less<> {
private:
  non_default_constructible_comparator() = default;

public:
  static non_default_constructible_comparator create() noexcept {
    return {};
  }

  non_default_constructible_comparator(const non_default_constructible_comparator&) = default;
  non_default_constructible_comparator(non_default_constructible_comparator&&) = default;

  non_default_constructible_comparator& operator=(const non_default_constructible_comparator&) = default;
  non_default_constructible_comparator& operator=(non_default_constructible_comparator&&) = default;

  ~non_default_constructible_comparator() = default;
};

class incomparable_int_custom_comparator;

class incomparable_int {
public:
  incomparable_int(int value)
      : val(value) {}

private:
  int val;

  friend class incomparable_int_custom_comparator;
};

class incomparable_int_custom_comparator {
public:
  bool operator()(const incomparable_int& lhs, const incomparable_int& rhs) const {
    return lhs.val < rhs.val;
  }
};

class expiring_comparator {
public:
  expiring_comparator() = default;

  expiring_comparator(const expiring_comparator&) = default;

  expiring_comparator(expiring_comparator&& other) noexcept
      : has_expired(std::exchange(other.has_expired, true)) {}

  expiring_comparator& operator=(const expiring_comparator&) = default;

  expiring_comparator& operator=(expiring_comparator&& other) noexcept {
    if (this != &other) {
      has_expired = std::exchange(other.has_expired, true);
    }
    return *this;
  }

  ~expiring_comparator() = default;

  template <typename L, typename R>
  bool operator()(L&& left, R&& right) const {
    if (has_expired) {
      throw std::runtime_error("Attempt to call an expired comparator");
    }
    return std::less<>()(std::forward<L>(left), std::forward<R>(right));
  }

private:
  bool has_expired = false;
};

class tracking_comparator {
public:
  explicit tracking_comparator(bool* called)
      : called(called) {}

  template <typename L, typename R>
  bool operator()(L&& left, R&& right) const {
    *called = true;
    return std::less<>()(std::forward<L>(left), std::forward<R>(right));
  }

private:
  bool* called;
};
