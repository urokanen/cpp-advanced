#include "bimap.h"
#include "fault-injection.h"
#include "test-classes.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>

namespace {

class element {
public:
  element(int data = 0)
      : value(data) {
    fault_injection_point();
  }

  element(const element& other)
      : value(-1) {
    fault_injection_point();
    value = other.value;
  }

  element& operator=(const element& other) = delete;

  ~element() = default;

  [[maybe_unused]] friend bool operator==(const element& lhs, const element& rhs) {
    fault_injection_point();
    return lhs.value == rhs.value;
  }

  [[maybe_unused]] friend auto operator<=>(const element& lhs, const element& rhs) {
    fault_injection_point();
    return lhs.value <=> rhs.value;
  }

  [[maybe_unused]] friend std::ostream& operator<<(std::ostream& out, const element& e) {
    return out << e.value;
  }

private:
  int value;
};

template <typename... Ts>
class snapshot {
public:
  explicit snapshot(const Ts&... objects)
      : snapshots(objects...) {}

  snapshot(const snapshot&) = delete;

  void verify(const Ts&... objects) const {
    std::apply([&](const snapshot<Ts>&... snapshots) { (snapshots.verify(objects), ...); }, snapshots);
  }

private:
  std::tuple<snapshot<Ts>...> snapshots;
};

template <>
class snapshot<bimap<element, element>> {
public:
  explicit snapshot(const bimap<element, element>& b)
      : bimap_snapshot(b) {}

  snapshot(const snapshot&) = delete;

  void verify(const bimap<element, element>& other) const {
    REQUIRE(other == bimap_snapshot);
  }

  bimap<element, element> bimap_snapshot;
};

template <typename F, typename... Ts>
void strong_exception_safety(F f, const Ts&... objects) {
  static_assert(sizeof...(objects) > 0, "Doesn't make sense without objects");

  fault_injection_disable dg;
  snapshot s(objects...);
  dg.reset();
  try {
    if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
      f();
    } else {
      [[maybe_unused]] volatile auto res = f();
    }
  } catch (...) {
    fault_injection_disable dg2;
    s.verify(objects...);
    throw;
  }
}

} // namespace

TEST_CASE("Default constructor does not throw") {
  assert_nothrow([] { [[maybe_unused]] bimap<element, element> b; });
}

TEST_CASE("Insert is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    strong_exception_safety([&a] { a.insert(1, 2); }, a);
    strong_exception_safety([&a] { a.insert(3, 4); }, a);
    strong_exception_safety([&a] { a.insert(5, 6); }, a);
    strong_exception_safety([&a] { a.insert(7, 8); }, a);
    strong_exception_safety([&a] { a.insert(9, 10); }, a);
  });
}

TEST_CASE("Copy constructor is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);
    }

    strong_exception_safety([&a] { return bimap<element, element>(a); }, a);
  });
}

TEST_CASE("Copy assignment to empty is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);
    }

    bimap<element, element> b;
    strong_exception_safety([&a, &b] { b = a; }, a, b);
  });
}

TEST_CASE("Copy assignment to non-empty is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    bimap<element, element> b;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);

      b.insert(1, 4);
      b.insert(8, 8);
      b.insert(25, 17);
      b.insert(13, 37);
    }

    strong_exception_safety([&a, &b] { b = a; }, a, b);
  });
}

TEST_CASE("Move constructor does not throw") {
  assert_nothrow([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);
    }

    strong_exception_safety([&a] { return bimap<element, element>(std::move(a)); }, a);
  });
}

TEST_CASE("Move assignment does not throw") {
  assert_nothrow([] {
    bimap<element, element> a;
    bimap<element, element> b;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);

      b.insert(1, 4);
      b.insert(8, 8);
      b.insert(25, 17);
      b.insert(13, 37);
    }

    strong_exception_safety([&a, &b] { b = std::move(a); }, a, b);
  });
}

TEST_CASE("Swap does not throw") {
  assert_nothrow([] {
    bimap<element, element> a;
    bimap<element, element> b;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);

      b.insert(1, 4);
      b.insert(8, 8);
      b.insert(25, 17);
      b.insert(13, 37);
    }

    strong_exception_safety([&a, &b] { swap(a, b); }, a, b);
  });
}

TEST_CASE("Erase by value is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);
    }

    a.erase_left(3);
    a.erase_right(a.find_right(8));
    a.erase_left(a.begin_left(), a.end_left());
  });
}

TEST_CASE("Erase by key does not throw") {
  assert_nothrow([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(1, 2);
      a.insert(3, 4);
      a.insert(5, 6);
      a.insert(7, 8);
      a.insert(9, 10);
    }

    strong_exception_safety([&a] { a.erase_left(std::next(a.begin_left(), 2)); }, a);
    strong_exception_safety([&a] { a.erase_right(a.begin_right(), a.end_right()); }, a);
  });
}

TEST_CASE("At-or-default is exception-safe") {
  faulty_run([] {
    bimap<element, element> a;
    {
      fault_injection_disable dg;
      a.insert(4, 2);
    }

    strong_exception_safety([&a] { a.at_left_or_default(4); }, a);
    strong_exception_safety([&a] { a.at_right_or_default(2); }, a);
    strong_exception_safety([&a] { a.at_left_or_default(5); }, a);
    strong_exception_safety([&a] { a.at_right_or_default(1); }, a);
    strong_exception_safety([&a] { a.at_left_or_default(42); }, a);
    strong_exception_safety([&a] { a.at_right_or_default(1000); }, a);
  });
}
