#include "optional.h"

#include "test-object.h"

#include <gtest/gtest.h>

namespace {

struct only_copy_constructible : test_object {
  using test_object::test_object;

  only_copy_constructible(const only_copy_constructible&) = default;
  only_copy_constructible& operator=(const only_copy_constructible&) = delete;

  only_copy_constructible(only_copy_constructible&& other) = delete;
  only_copy_constructible& operator=(only_copy_constructible&&) = delete;
};

struct only_move_constructible : test_object {
  using test_object::test_object;

  only_move_constructible(const only_move_constructible&) = delete;
  only_move_constructible& operator=(const only_move_constructible&) = delete;

  only_move_constructible(only_move_constructible&& other) = default;
  only_move_constructible& operator=(only_move_constructible&&) = delete;
};

struct only_copyable : test_object {
  using test_object::test_object;

  only_copyable(const only_copyable& other)
      : test_object(other) {
    ++copy_ctor_calls;
  }

  only_copyable& operator=(const only_copyable& other) {
    test_object::operator=(other);
    ++copy_assign_calls;
    return *this;
  }

  only_copyable(only_copyable&& other) = delete;
  only_copyable& operator=(only_copyable&& other) = delete;

  inline static size_t copy_ctor_calls = 0;
  inline static size_t copy_assign_calls = 0;
};

struct only_movable : test_object {
  using test_object::test_object;

  only_movable(const only_movable&) = delete;
  only_movable& operator=(const only_movable&) = delete;

  only_movable(only_movable&& other)
      : test_object(std::move(other)) {
    ++move_ctor_calls;
  }

  only_movable& operator=(only_movable&& other) {
    test_object::operator=(std::move(other));
    ++move_assign_calls;
    return *this;
  }

  inline static size_t move_ctor_calls = 0;
  inline static size_t move_assign_calls = 0;
};

class optional_test : public ::testing::Test {
protected:
  void SetUp() noexcept override {
    only_copyable::copy_ctor_calls = 0;
    only_copyable::copy_assign_calls = 0;
    only_movable::move_ctor_calls = 0;
    only_movable::move_ctor_calls = 0;
  }

  test_object::no_new_instances_guard instances_guard;
};

} // namespace

const nullopt_t* get_nullopt_ptr() noexcept {
  return &nullopt;
}

const in_place_t* get_in_place_ptr() noexcept {
  return &in_place;
}

TEST_F(optional_test, default_ctor) {
  optional<test_object> a;
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
}

TEST_F(optional_test, default_ctor_no_instances) {
  optional<test_object> a;
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
  instances_guard.expect_no_instances();
}

TEST_F(optional_test, value_ctor) {
  optional<test_object> a(42);
  EXPECT_TRUE(a.has_value());
  EXPECT_TRUE(static_cast<bool>(a));
  EXPECT_EQ(*a, 42);
}

TEST_F(optional_test, value_ctor_explicit_conversion) {
  optional<std::string> a(std::string_view("hello"));
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, "hello");
}

TEST_F(optional_test, value_ctor_implicit_conversion) {
  optional<std::string> a = "hello";
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, "hello");
}

TEST_F(optional_test, value_ctor_lvalue_reference) {
  struct ref_wrapper {
    ref_wrapper(int& x)
        : value(x) {}

    int& value;
  };

  int x = 42;
  optional<ref_wrapper> a = x;
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(a->value, 42);

  a->value = 55;
  EXPECT_EQ(x, 55);
}

TEST_F(optional_test, dereference) {
  optional<test_object> a(42);
  EXPECT_EQ(*a, 42);
  EXPECT_EQ(*std::as_const(a), 42);
  EXPECT_EQ(*std::move(a), 42);
  EXPECT_EQ(*std::move(std::as_const(a)), 42);
}

TEST_F(optional_test, member_access) {
  optional<test_object> a(42);
  EXPECT_EQ(a->operator int(), 42);
  EXPECT_EQ(std::as_const(a)->operator int(), 42);
}

TEST_F(optional_test, hijacked_addressof) {
  struct value_type {
    int x;

    void* operator&() = delete;
  };

  optional<value_type> a(value_type{42});
  EXPECT_TRUE(a->x == 42);
  EXPECT_TRUE(a->x == (*a).x);
  EXPECT_TRUE(&a->x == &(*a).x);
  EXPECT_TRUE(std::as_const(a)->x == 42);
  EXPECT_TRUE(&std::as_const(a)->x == &(*a).x);
}

TEST_F(optional_test, reset) {
  optional<test_object> a(42);
  EXPECT_TRUE(a.has_value());
  a.reset();
  EXPECT_FALSE(a.has_value());
  instances_guard.expect_no_instances();
}

TEST_F(optional_test, dtor) {
  optional<test_object> a(42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 42);
}

TEST_F(optional_test, copy_ctor) {
  optional<only_copy_constructible> a(in_place, 42);
  optional<only_copy_constructible> b = a;
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
}

TEST_F(optional_test, copy_ctor_empty) {
  optional<only_copy_constructible> a;
  optional<only_copy_constructible> b = a;
  EXPECT_FALSE(b.has_value());
}

TEST_F(optional_test, move_ctor) {
  optional<only_move_constructible> a(in_place, 42);
  optional<only_move_constructible> b = std::move(a);
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 42);
}

TEST_F(optional_test, move_ctor_empty) {
  optional<only_move_constructible> a;
  optional<only_move_constructible> b = std::move(a);
  EXPECT_FALSE(b.has_value());
  EXPECT_FALSE(a.has_value());
}

TEST_F(optional_test, copy_assignment_empty_empty) {
  optional<only_copyable> a, b;
  b = a;
  EXPECT_FALSE(b.has_value());
  EXPECT_EQ(only_copyable::copy_ctor_calls, 0);
  EXPECT_EQ(only_copyable::copy_assign_calls, 0);
}

TEST_F(optional_test, copy_assignment_to_empty) {
  optional<only_copyable> a(in_place, 42), b;
  b = a;
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
  EXPECT_EQ(only_copyable::copy_ctor_calls, 1);
  EXPECT_EQ(only_copyable::copy_assign_calls, 0);
}

TEST_F(optional_test, copy_assignment_from_empty) {
  optional<only_copyable> a, b(in_place, 42);
  b = a;
  EXPECT_FALSE(b.has_value());
  EXPECT_EQ(only_copyable::copy_ctor_calls, 0);
  EXPECT_EQ(only_copyable::copy_assign_calls, 0);
}

TEST_F(optional_test, copy_assignment) {
  optional<only_copyable> a(in_place, 42), b(in_place, 41);
  b = a;
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
  EXPECT_EQ(only_copyable::copy_ctor_calls, 0);
  EXPECT_EQ(only_copyable::copy_assign_calls, 1);
}

TEST_F(optional_test, move_assignment_empty_empty) {
  optional<only_movable> a, b;
  b = std::move(a);
  EXPECT_FALSE(b.has_value());
  EXPECT_FALSE(a.has_value());
  EXPECT_EQ(only_movable::move_ctor_calls, 0);
  EXPECT_EQ(only_movable::move_assign_calls, 0);
}

TEST_F(optional_test, move_assignment_to_empty) {
  optional<only_movable> a(in_place, 42), b;
  b = std::move(a);
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 42);
  EXPECT_EQ(only_movable::move_ctor_calls, 1);
  EXPECT_EQ(only_movable::move_assign_calls, 0);
}

TEST_F(optional_test, move_assignment_from_empty) {
  optional<only_movable> a, b(in_place, 42);
  b = std::move(a);
  EXPECT_FALSE(b.has_value());
  EXPECT_FALSE(a.has_value());
  EXPECT_EQ(only_movable::move_ctor_calls, 0);
  EXPECT_EQ(only_movable::move_assign_calls, 0);
}

TEST_F(optional_test, move_assignment) {
  optional<only_movable> a(in_place, 42), b(in_place, 41);
  b = std::move(a);
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ(*b, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 42);
  EXPECT_EQ(only_movable::move_ctor_calls, 0);
  EXPECT_EQ(only_movable::move_assign_calls, 1);
}

TEST_F(optional_test, nullopt_ctor) {
  {
    optional<test_object> a = nullopt;
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(static_cast<bool>(a));
    instances_guard.expect_no_instances();
  }
  {
    optional<test_object> a = {nullopt};
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(static_cast<bool>(a));
    instances_guard.expect_no_instances();
  }
  {
    optional<test_object> a{nullopt};
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(static_cast<bool>(a));
    instances_guard.expect_no_instances();
  }
}

TEST_F(optional_test, nullopt_assignment) {
  optional<test_object> a(42);
  a = nullopt;
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
  EXPECT_TRUE(noexcept(a = nullopt));
  instances_guard.expect_no_instances();
}

TEST_F(optional_test, empty_ctor_int) {
  optional<int> a = {};
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
}

TEST_F(optional_test, empty_ctor_struct_with_default_ctor) {
  optional<test_object> a = {};
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
}

TEST_F(optional_test, empty_assignment_int) {
  optional<int> a(42);
  a = {};
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
}

TEST_F(optional_test, empty_assignment_struct_with_default_ctor) {
  struct A {};

  optional<A> a(A{});
  a = {};
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
}

TEST_F(optional_test, empty_assignment_struct_without_default_ctor) {
  optional<test_object> a(42);
  a = {};
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
  instances_guard.expect_no_instances();
}

TEST_F(optional_test, value_assignment) {
  optional<int> a(42);

  a = 55;
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 55);

  short s = 1337;
  a = s;
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 1337);
}

TEST_F(optional_test, swap_non_empty) {
  optional<test_object> a(42);
  optional<test_object> b(55);

  swap(a, b);

  EXPECT_EQ(*a, 55);
  EXPECT_EQ(*b, 42);
}

TEST_F(optional_test, swap_empty_right) {
  optional<test_object> a(42);
  optional<test_object> b;

  swap(a, b);

  EXPECT_FALSE(a);
  EXPECT_EQ(*b, 42);
}

TEST_F(optional_test, swap_empty_left) {
  optional<test_object> a;
  optional<test_object> b(55);

  swap(a, b);

  EXPECT_EQ(*a, 55);
  EXPECT_FALSE(b);
}

TEST_F(optional_test, swap_empty_both) {
  optional<test_object> a;
  optional<test_object> b;

  swap(a, b);

  EXPECT_FALSE(a);
  EXPECT_FALSE(b);
}

namespace {

struct custom_swap {
  custom_swap(int value) noexcept
      : value(value) {}

  custom_swap(const custom_swap&) = delete;
  custom_swap(custom_swap&&) = default;

  custom_swap& operator=(const custom_swap&) = delete;
  custom_swap& operator=(custom_swap&&) = delete;

  friend void swap(custom_swap& lhs, custom_swap& rhs) noexcept {
    std::swap(lhs.value, rhs.value);
    ++lhs.value;
    ++rhs.value;
  }

  int value;
};

} // namespace

TEST_F(optional_test, swap_custom) {
  optional<custom_swap> a(42);
  optional<custom_swap> b(55);

  swap(a, b);

  EXPECT_EQ(a->value, 56);
  EXPECT_EQ(b->value, 43);
}

TEST_F(optional_test, swap_empty_custom) {
  optional<custom_swap> a(42);
  optional<custom_swap> b;

  swap(a, b);

  EXPECT_FALSE(a);
  EXPECT_EQ(b->value, 42);
}

namespace {

struct non_default_constructor {
  non_default_constructor(int, int, int, std::unique_ptr<int>) {}
};

} // namespace

TEST_F(optional_test, in_place_ctor) {
  optional<non_default_constructor> a(in_place, 1, 2, 3, std::unique_ptr<int>());
  EXPECT_TRUE(a.has_value());
}

TEST_F(optional_test, emplace) {
  optional<non_default_constructor> a;
  non_default_constructor& result = a.emplace(1, 2, 3, std::unique_ptr<int>());
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(&result, &*a);
}

TEST_F(optional_test, emplace_throw) {
  struct throwing_ctor {
    struct exception : std::exception {
      using std::exception::exception;
    };

    throwing_ctor(bool should_throw, int) {
      if (should_throw) {
        throw exception();
      }
    }
  };

  optional<throwing_ctor> a(in_place, false, 42);
  EXPECT_THROW(a.emplace(true, 55), throwing_ctor::exception);
  EXPECT_FALSE(a.has_value());
}

TEST_F(optional_test, copy_assignment_throw) {
  struct throwing_copy {
    struct exception : std::exception {
      using std::exception::exception;
    };

    throwing_copy(bool should_throw_when_copied, int value)
        : should_throw_when_copied(should_throw_when_copied)
        , value(value) {}

    throwing_copy(const throwing_copy& other) {
      if (other.should_throw_when_copied) {
        throw exception();
      }
    }

    throwing_copy& operator=(const throwing_copy&) = default;

    bool should_throw_when_copied;
    int value;
  };

  {
    optional<throwing_copy> a(in_place, false, 42);
    EXPECT_NO_THROW(a = throwing_copy(true, 55));
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a->value, 55);
  }
  {
    optional<throwing_copy> b;
    EXPECT_THROW(b = throwing_copy(true, 55), throwing_copy::exception);
    EXPECT_FALSE(b.has_value());
  }
}

namespace {

struct comparison_counters {
  size_t equal = 0;
  size_t not_equal = 0;
  size_t less = 0;
  size_t less_equal = 0;
  size_t greater = 0;
  size_t greater_equal = 0;
};

struct custom_comparison {
  custom_comparison(int value, comparison_counters* counters)
      : value(value)
      , counters(counters) {}

  bool operator==(const custom_comparison& other) const {
    ++counters->equal;
    return this->value == other.value;
  }

  bool operator!=(const custom_comparison& other) const {
    ++counters->not_equal;
    return this->value != other.value;
  }

  bool operator<(const custom_comparison& other) const {
    ++counters->less;
    return this->value < other.value;
  }

  bool operator<=(const custom_comparison& other) const {
    ++counters->less_equal;
    return this->value <= other.value;
  }

  bool operator>(const custom_comparison& other) const {
    ++counters->greater;
    return this->value > other.value;
  }

  bool operator>=(const custom_comparison& other) const {
    ++counters->greater_equal;
    return this->value >= other.value;
  }

private:
  int value;
  comparison_counters* counters;
};

} // namespace

TEST_F(optional_test, comparison_non_empty_and_non_empty) {
  comparison_counters ca, cb;
  optional<custom_comparison> a(in_place, 41, &ca);
  optional<custom_comparison> b(in_place, 42, &cb);

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(a < b);
  EXPECT_TRUE(a <= b);
  EXPECT_FALSE(a > b);
  EXPECT_FALSE(a >= b);

  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
  EXPECT_FALSE(a < a);
  EXPECT_TRUE(a <= a);
  EXPECT_FALSE(a > a);
  EXPECT_TRUE(a >= a);

  EXPECT_FALSE(b == a);
  EXPECT_TRUE(b != a);
  EXPECT_FALSE(b < a);
  EXPECT_FALSE(b <= a);
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(b >= a);

  EXPECT_EQ(ca.equal, 2);
  EXPECT_EQ(ca.not_equal, 2);
  EXPECT_EQ(ca.less, 2);
  EXPECT_EQ(ca.less_equal, 2);
  EXPECT_EQ(ca.greater, 2);
  EXPECT_EQ(ca.greater_equal, 2);

  EXPECT_EQ(cb.equal, 1);
  EXPECT_EQ(cb.not_equal, 1);
  EXPECT_EQ(cb.less, 1);
  EXPECT_EQ(cb.less_equal, 1);
  EXPECT_EQ(cb.greater, 1);
  EXPECT_EQ(cb.greater_equal, 1);
}

TEST_F(optional_test, comparison_non_empty_and_empty) {
  comparison_counters ca;
  optional<custom_comparison> a(in_place, 41, &ca);
  optional<custom_comparison> b;

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(a <= b);
  EXPECT_TRUE(a > b);
  EXPECT_TRUE(a >= b);

  EXPECT_FALSE(b == a);
  EXPECT_TRUE(b != a);
  EXPECT_TRUE(b < a);
  EXPECT_TRUE(b <= a);
  EXPECT_FALSE(b > a);
  EXPECT_FALSE(b >= a);

  EXPECT_EQ(ca.equal, 0);
  EXPECT_EQ(ca.not_equal, 0);
  EXPECT_EQ(ca.less, 0);
  EXPECT_EQ(ca.less_equal, 0);
  EXPECT_EQ(ca.greater, 0);
  EXPECT_EQ(ca.greater_equal, 0);
}

TEST_F(optional_test, comparison_empty_and_empty) {
  optional<custom_comparison> a, b;

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(a < b);
  EXPECT_TRUE(a <= b);
  EXPECT_FALSE(a > b);
  EXPECT_TRUE(a >= b);

  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
  EXPECT_FALSE(a < a);
  EXPECT_TRUE(a <= a);
  EXPECT_FALSE(a > a);
  EXPECT_TRUE(a >= a);

  EXPECT_TRUE(b == a);
  EXPECT_FALSE(b != a);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(b <= a);
  EXPECT_FALSE(b > a);
  EXPECT_TRUE(b >= a);
}

TEST_F(optional_test, comparison_three_way) {
  static_assert(std::is_eq(optional<int>{} <=> optional<int>{}));
  static_assert(std::is_lt(optional<int>{} <=> optional<int>{1}));
  static_assert(std::is_gt(optional<int>{1} <=> optional<int>{}));
  static_assert(std::is_eq(optional<int>{1} <=> optional<int>{1}));
  static_assert(std::is_lt(optional<int>{1} <=> optional<int>{2}));
}

TEST_F(optional_test, type_deduction) {
  {
    optional opt = 42;
    static_assert(std::is_same_v<decltype(opt), optional<int>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 42);
  }
  {
    int x = 42;
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<int>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 42);
  }
  {
    const int x = 42;
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<int>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 42);
  }
  {
    optional x = 42;
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<int>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 42);
  }
  {
    struct dummy {};

    optional opt = dummy{};
    static_assert(std::is_same_v<decltype(opt), optional<dummy>>);
    EXPECT_TRUE(opt.has_value());
  }
  {
    const int& x = 42;
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<int>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 42);
  }
  {
    const int* x = nullptr;
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<const int*>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, nullptr);
  }
  {
    int x[] = {1, 2, 3};
    optional opt = x;
    static_assert(std::is_same_v<decltype(opt), optional<int*>>);
    EXPECT_TRUE(opt.has_value());
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ((*opt)[0], 1);
  }
  {
    optional opt1 = only_movable(42);
    static_assert(std::is_same_v<decltype(opt1), optional<only_movable>>);

    optional opt2 = std::move(opt1);
    static_assert(std::is_same_v<decltype(opt2), optional<only_movable>>);
    EXPECT_TRUE(opt2.has_value());
    EXPECT_EQ(*opt2, 42);
  }
}
