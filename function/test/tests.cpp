#include "function.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

TEST(function_test, default_ctor) {
  function<void()> x;
  function<void(int, int, int)> y;
  function<double && (float&, const int&, int)> z;
}

TEST(function_test, empty_convertion_to_bool) {
  function<void()> x;
  EXPECT_FALSE(static_cast<bool>(x));
}

TEST(function_test, empty_call) {
  function<void()> x;
  EXPECT_THROW(x(), bad_function_call);
  try {
    x();
  } catch (const std::exception& e) {
    std::exception basic_exception;
    EXPECT_NE(e.what(), basic_exception.what());
  }
}

TEST(function_test, empty_copy_move) {
  function<void()> x;

  function<void()> y = x;
  EXPECT_FALSE(static_cast<bool>(y));

  function<void()> z = std::move(x);
  EXPECT_FALSE(static_cast<bool>(z));

  z = y;
  EXPECT_FALSE(static_cast<bool>(z));

  y = std::move(z);
  EXPECT_FALSE(static_cast<bool>(y));
}

TEST(function_test, lambda) {
  function<int()> f = [] {
    return 42;
  };
  EXPECT_EQ(f(), 42);
}

TEST(function_test, pointer_to_function) {
  function<int()> f = +[] {
    return 42;
  };
  EXPECT_EQ(f(), 42);
}

TEST(function_test, copy_ctor) {
  function<int()> f = [] {
    return 42;
  };
  function<int()> g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

namespace {

template <typename T, typename F>
bool is_small(const F& f) {
  return std::less_equal<>{}(static_cast<const void*>(&f), static_cast<const void*>(f.template target<T>())) &&
         std::less<>{}(static_cast<const void*>(f.template target<T>()), static_cast<const void*>(&f + 1));
}

struct small_func {
  explicit small_func(int value) noexcept
      : value(value) {}

  small_func(const small_func&) = default;
  small_func& operator=(const small_func&) = default;

  small_func(small_func&& other) noexcept {
    *this = std::move(other);
  }

  small_func& operator=(small_func&& other) noexcept {
    value = std::exchange(other.value, 0);
    return *this;
  }

  int operator()() const {
    return value;
  }

  int get_value() const {
    return value;
  }

  int set_value(int new_value) {
    return value = new_value;
  }

private:
  int value;
};

} // namespace

TEST(function_test, empty_target) {
  function<int()> f;
  EXPECT_EQ(f.target<small_func>(), nullptr);
  EXPECT_EQ(std::as_const(f).target<small_func>(), nullptr);
}

TEST(function_test, small_func) {
  function<int()> f = small_func(42);
  EXPECT_EQ(f(), 42);
  EXPECT_TRUE(is_small<small_func>(f));
}

TEST(function_test, small_func_const) {
  const function<int()> f = small_func(42);
  EXPECT_EQ(f(), 42);
  EXPECT_TRUE(is_small<small_func>(f));
}

TEST(function_test, small_func_target) {
  function<int()> f = small_func(42);
  EXPECT_EQ(f.target<small_func>()->get_value(), 42);
  EXPECT_EQ(std::as_const(f).target<small_func>()->get_value(), 42);
}

TEST(function_test, small_func_copy_ctor) {
  function<int()> f = small_func(42);
  function<int()> g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, small_func_copy_ctor_independence) {
  function<int()> f = small_func(42);
  function<int()> g = f;
  f.target<small_func>()->set_value(55);
  EXPECT_EQ(f(), 55);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, small_func_move_ctor) {
  function<int()> f = small_func(42);
  function<int()> g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_FALSE(f);
}

TEST(function_test, small_func_copy_assignment) {
  function<int()> f = small_func(42);
  function<int()> g = small_func(55);
  g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, small_func_copy_assignment_to_empty) {
  function<int()> f = small_func(42);
  function<int()> g;
  g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, small_func_copy_assignment_from_empty) {
  function<int()> f;
  function<int()> g = small_func(55);
  g = f;
  EXPECT_FALSE(f);
  EXPECT_FALSE(g);
}

TEST(function_test, small_func_copy_assignment_independence) {
  function<int()> f = small_func(42);
  function<int()> g;
  g = f;
  f.target<small_func>()->set_value(55);
  EXPECT_EQ(f(), 55);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, small_func_move_assignment) {
  function<int()> f = small_func(42);
  function<int()> g = small_func(55);
  g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_TRUE(!f || f() == 55);
}

TEST(function_test, small_func_move_assignment_to_empty) {
  function<int()> f = small_func(42);
  function<int()> g;
  g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_FALSE(f);
}

TEST(function_test, small_func_move_assignment_from_empty) {
  function<int()> f;
  function<int()> g = small_func(55);
  g = std::move(f);
  EXPECT_FALSE(g);
}

TEST(function_test, small_func_copy_assignment_self) {
  function<int()> f = small_func(42);
  f = f;
  EXPECT_EQ(f(), 42);
}

TEST(function_test, small_func_move_assignment_self) {
  function<int()> f = small_func(42);
  f = std::move(f);
  EXPECT_EQ(f(), 42);
}

namespace {

struct small_func_with_pointer {
  explicit small_func_with_pointer()
      : pointer(this) {}

  void swap(small_func_with_pointer& other) noexcept {
    std::swap(pointer, other.pointer);
  }

  small_func_with_pointer(const small_func_with_pointer&) noexcept
      : pointer(this) {}

  small_func_with_pointer& operator=(const small_func_with_pointer& other) noexcept {
    if (this != &other) {
      small_func_with_pointer(other).swap(*this);
    }
    return *this;
  }

  small_func_with_pointer(small_func_with_pointer&&) noexcept
      : pointer(this) {}

  small_func_with_pointer& operator=(small_func_with_pointer&& other) noexcept {
    if (this != &other) {
      small_func_with_pointer(std::move(other)).swap(*this);
    }
    return *this;
  }

  bool operator()() const {
    return pointer == this;
  }

private:
  small_func_with_pointer* pointer;
};

} // namespace

TEST(function_test, small_func_with_pointer) {
  function<int()> f = small_func_with_pointer();
  EXPECT_TRUE(is_small<small_func_with_pointer>(f));
}

TEST(function_test, small_func_with_pointer_copy_ctor) {
  function<int()> f = small_func_with_pointer();
  function<int()> g(f);
  EXPECT_TRUE(f());
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_copy_assignment) {
  function<int()> f = small_func_with_pointer();
  function<int()> g = small_func_with_pointer();
  f = g;
  EXPECT_TRUE(f());
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_move_ctor) {
  function<int()> f = small_func_with_pointer();
  function<int()> g(std::move(f));
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_move_assignment) {
  function<int()> f = small_func_with_pointer();
  function<int()> g = small_func_with_pointer();
  g = std::move(f);
  EXPECT_TRUE(g());
}

namespace {

struct large_func {
  large_func(int value) noexcept
      : that(this)
      , value(value) {
    ++n_instances;
  }

  large_func(const large_func& other) noexcept
      : that(this)
      , value(other.value) {
    ++n_instances;
  }

  large_func& operator=(const large_func& rhs) noexcept {
    value = rhs.value;
    return *this;
  }

  ~large_func() {
    assert(this == that);
    --n_instances;
  }

  int operator()() const noexcept {
    assert(this == that);
    return value;
  }

  static void assert_no_instances() {
    assert(n_instances == 0);
  }

  int get_value() const {
    return value;
  }

  int set_value(int new_value) {
    return value = new_value;
  }

private:
  [[maybe_unused]] large_func* that;
  int value;
  [[maybe_unused]] int payload[1000]{};

  inline static size_t n_instances = 0;
};

} // namespace

TEST(function_test, large_func) {
  {
    function<int()> f = large_func(42);
    EXPECT_EQ(f(), 42);
  }
  large_func::assert_no_instances();
}

TEST(function_test, large_func_const) {
  {
    const function<int()> f = large_func(42);
    EXPECT_EQ(f(), 42);
  }
  large_func::assert_no_instances();
}

TEST(function_test, large_func_target) {
  function<int()> f = large_func(42);
  EXPECT_EQ(f.target<large_func>()->get_value(), 42);
  EXPECT_EQ(std::as_const(f).target<large_func>()->get_value(), 42);
}

TEST(function_test, large_func_copy_ctor) {
  function<int()> f = large_func(42);
  function<int()> g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, large_func_copy_ctor_independence) {
  function<int()> f = large_func(42);
  function<int()> g = f;
  f.target<large_func>()->set_value(55);
  EXPECT_EQ(f(), 55);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, large_func_move_ctor) {
  function<int()> f = large_func(42);
  function<int()> g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_FALSE(f);
}

TEST(function_test, large_func_copy_assignment) {
  function<int()> f = large_func(42);
  function<int()> g = large_func(55);
  g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, large_func_copy_assignment_to_empty) {
  function<int()> f = large_func(42);
  function<int()> g;
  g = f;
  EXPECT_EQ(f(), 42);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, large_func_copy_assignment_from_empty) {
  function<int()> f;
  function<int()> g = large_func(55);
  g = f;
  EXPECT_FALSE(f);
  EXPECT_FALSE(g);
}

TEST(function_test, large_func_copy_assignment_independence) {
  function<int()> f = large_func(42);
  function<int()> g;
  g = f;
  f.target<large_func>()->set_value(55);
  EXPECT_EQ(f(), 55);
  EXPECT_EQ(g(), 42);
}

TEST(function_test, large_func_move_assignment) {
  function<int()> f = large_func(42);
  function<int()> g = large_func(55);
  g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_TRUE(!f || f() == 55);
}

TEST(function_test, large_func_move_assignment_to_empty) {
  function<int()> f = large_func(42);
  function<int()> g;
  g = std::move(f);
  EXPECT_EQ(g(), 42);
  EXPECT_FALSE(f);
}

TEST(function_test, large_func_move_assignment_from_empty) {
  function<int()> f;
  function<int()> g = large_func(55);
  g = std::move(f);
  EXPECT_FALSE(g);
}

TEST(function_test, large_func_copy_assignment_self) {
  function<int()> f = large_func(42);
  f = f;
  EXPECT_EQ(f(), 42);
}

TEST(function_test, large_func_move_assignment_self) {
  function<int()> f = large_func(42);
  f = std::move(f);
  EXPECT_EQ(f(), 42);
}

namespace {

struct throwing_copy {
  struct exception final : std::exception {
    using std::exception::exception;
  };

  throwing_copy() = default;

  int operator()() const {
    return 43;
  }

  throwing_copy(const throwing_copy&) {
    throw exception();
  }

  throwing_copy(throwing_copy&&) noexcept {}

  throwing_copy& operator=(const throwing_copy&) = delete;
  throwing_copy& operator=(throwing_copy&&) = delete;
};

} // namespace

TEST(function_test, small_func_throwing_copy) {
  function<int()> f = small_func(42);
  function<int()> g = throwing_copy();

  EXPECT_TRUE(is_small<throwing_copy>(g));

  EXPECT_THROW(f = g, throwing_copy::exception);
  EXPECT_EQ(f(), 42);

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(f(), 43);
}

TEST(function_test, large_func_throwing_copy) {
  function<int()> f = large_func(42);
  function<int()> g = throwing_copy();

  EXPECT_TRUE(is_small<throwing_copy>(g));

  EXPECT_THROW(f = g, throwing_copy::exception);
  EXPECT_EQ(f(), 42);

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(f(), 43);
}

namespace {

struct throwing_move {
  struct exception final : std::exception {
    using std::exception::exception;
  };

  throwing_move() = default;

  int operator()() const {
    return 43;
  }

  throwing_move(const throwing_move& other)
      : enable_exception(other.enable_exception) {
    throw exception();
  }

  throwing_move(throwing_move&& other)
      : enable_exception(other.enable_exception) {
    if (enable_exception) {
      throw exception();
    }
  }

  throwing_move& operator=(const throwing_move&) = delete;
  throwing_move& operator=(throwing_move&&) = delete;

  bool enable_exception = false;
};

} // namespace

TEST(function_test, throwing_move) {
  function<int()> f = small_func(42);
  function<int()> g = throwing_move();

  g.target<throwing_move>()->enable_exception = true;

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(f(), 43);
}

TEST(function_test, arguments) {
  function<int(int, int)> f = [](int a, int b) {
    return a + b;
  };

  int x = 2;
  EXPECT_EQ(f(40, x), 42);
}

TEST(function_test, arguments_ref) {
  function<int&(int&)> f = [](int& a) -> int& {
    return a;
  };

  int x = 42;
  EXPECT_EQ(&x, &f(x));
}

TEST(function_test, arguments_cref) {
  function<const int&(const int&)> f = [](const int& a) -> const int& {
    return a;
  };

  const int x = 42;
  EXPECT_EQ(&x, &f(x));
}

namespace {

struct non_copyable {
  non_copyable() {}

  non_copyable(const non_copyable&) = delete;
  non_copyable(non_copyable&&) = default;
};

} // namespace

TEST(function_test, argument_by_value) {
  function<non_copyable(non_copyable)> f = [](non_copyable a) {
    return std::move(a);
  };
  [[maybe_unused]] non_copyable a = f(non_copyable());
}

TEST(function_test, argument_by_value_large) {
  int big_array[1000]{};
  function<non_copyable(non_copyable)> f = [big_array](non_copyable a) {
    (void) big_array;
    return std::move(a);
  };
  [[maybe_unused]] non_copyable a = f(non_copyable());
}

TEST(function_test, recursive_test) {
  function<int(int)> fib = [&fib](int n) -> int {
    switch (n) {
    case 0:
      return 0;
    case 1:
      return 1;
    default:
      return fib(n - 1) + fib(n - 2);
    }
  };
  EXPECT_EQ(fib(10), 55);
}

TEST(function_test, target) {
  struct foo {
    void operator()() const {}
  };

  struct bar {
    void operator()() const {}
  };

  function<void()> f = foo();
  EXPECT_NE(f.target<foo>(), nullptr);
  EXPECT_EQ(f.target<bar>(), nullptr);
  EXPECT_NE(std::as_const(f).target<foo>(), nullptr);
  EXPECT_EQ(std::as_const(f).target<bar>(), nullptr);
  f = bar();
  EXPECT_EQ(f.target<foo>(), nullptr);
  EXPECT_NE(f.target<bar>(), nullptr);
  EXPECT_EQ(std::as_const(f).target<foo>(), nullptr);
  EXPECT_NE(std::as_const(f).target<bar>(), nullptr);
}

TEST(function_test, mutable_small) {
  function<int()> f = [x = 0]() mutable {
    return ++x;
  };
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);
}

TEST(function_test, mutable_small_const) {
  const function<int()> f = [x = 0]() mutable {
    return ++x;
  };
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);
}

TEST(function_test, mutable_large) {
  int big_array[1000]{};
  function<int()> f = [x = 0, big_array]() mutable {
    (void) big_array;
    return ++x;
  };
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);
}

TEST(function_test, mutable_large_const) {
  int big_array[1000]{};
  const function<int()> f = [x = 0, big_array]() mutable {
    (void) big_array;
    return ++x;
  };
  EXPECT_EQ(f(), 1);
  EXPECT_EQ(f(), 2);
}

struct tracking_func {
  struct tracker_type {
    size_t destructors = 0;
    size_t moves = 0;
  };

  tracking_func(tracker_type* tracker) noexcept
      : tracker(tracker) {}

  tracking_func(const tracking_func& other)
      : tracker(other.tracker) {
    throw std::logic_error("This constructor should never be called");
  }

  tracking_func(tracking_func&& other) noexcept
      : tracker(other.tracker) {
    ++tracker->moves;
  }

  tracking_func& operator=(const tracking_func&) = delete;
  tracking_func& operator=(tracking_func&&) = delete;

  void operator()() const noexcept {}

  ~tracking_func() {
    ++tracker->destructors;
  }

private:
  tracker_type* tracker;
};

struct large_tracking_func : tracking_func {
  using tracking_func::tracking_func;

  [[maybe_unused]] int payload[1000]{};
};

TEST(function_test, move_assignment_large_to_small) {
  tracking_func::tracker_type ts;
  tracking_func::tracker_type tl;

  {
    function<void()> small = tracking_func(&ts);
    EXPECT_TRUE(is_small<tracking_func>(small));
    EXPECT_EQ(ts.moves, 1);
    ts = {};

    function<void()> large = large_tracking_func(&tl);
    EXPECT_FALSE(is_small<large_tracking_func>(large));
    EXPECT_EQ(tl.moves, 1);
    tl = {};

    small = std::move(large);
    EXPECT_EQ(tl.moves, 0);
    EXPECT_EQ(tl.destructors, 0);
    EXPECT_LE(ts.destructors, 1);
  }

  EXPECT_EQ(tl.moves, 0);
  EXPECT_EQ(tl.destructors, 1);
  EXPECT_EQ(ts.destructors, ts.moves + 1);
}

TEST(function_test, move_assignment_small_to_large) {
  tracking_func::tracker_type ts;
  tracking_func::tracker_type tl;

  {
    function<void()> small = tracking_func(&ts);
    EXPECT_TRUE(is_small<tracking_func>(small));
    EXPECT_EQ(ts.moves, 1);
    ts = {};

    function<void()> large = large_tracking_func(&tl);
    EXPECT_FALSE(is_small<large_tracking_func>(large));
    EXPECT_EQ(tl.moves, 1);
    tl = {};

    large = std::move(small);
    EXPECT_EQ(tl.moves, 0);
    EXPECT_EQ(tl.destructors, 1);
    EXPECT_LE(ts.moves, 1);
  }

  EXPECT_EQ(tl.moves, 0);
  EXPECT_EQ(tl.destructors, 1);
  EXPECT_EQ(ts.destructors, ts.moves + 1);
}

TEST(function_test, arguments_efficient_forwarding) {
  tracking_func::tracker_type t;

  function<int(tracking_func)> f = [](tracking_func) {
    return 42;
  };

  EXPECT_EQ(f(tracking_func(&t)), 42);
  EXPECT_LE(t.moves, 1);
}

namespace {

template <size_t... Is, typename F>
void static_for_each(std::index_sequence<Is...>, F f) {
  (f.template operator()<Is>(), ...);
}

template <size_t SIZE>
struct sized_func {
  size_t operator()() const noexcept {
    return SIZE;
  }

  std::array<std::byte, SIZE> payload{};
};

} // namespace

TEST(function_test, different_sizes) {
  using base_sizes = std::index_sequence<1, 2, 4, 8, 16, 32, 64, 128, 256>;
  using multipliers = std::index_sequence<1, 3, 5, 7, 9, 11, 13, 15>;

  static_for_each(base_sizes{}, []<size_t BASE_SIZE>() {
    static_for_each(multipliers{}, []<size_t MULTIPLIER>() {
      static constexpr size_t SIZE = BASE_SIZE * MULTIPLIER;

      function<size_t()> f = sized_func<SIZE>();
      f.target<sized_func<SIZE>>()->payload.fill(std::byte{});
      EXPECT_EQ(f(), SIZE);
    });
  });
}

TEST(function_test, alignment) {
  using alignments = std::index_sequence<1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192>;

  static_for_each(alignments{}, []<size_t ALIGNMENT>() {
    using sizes = std::index_sequence<1, ALIGNMENT>;

    static_for_each(sizes{}, []<size_t SIZE>() {
      struct alignas(ALIGNMENT) aligned_func : sized_func<SIZE> {};

      function<size_t()> f = aligned_func();
      auto* target = f.target<aligned_func>();
      target->payload.fill(std::byte{});
      EXPECT_EQ(reinterpret_cast<uintptr_t>(target) % ALIGNMENT, 0);
      EXPECT_EQ(f(), SIZE);
    });
  });
}
