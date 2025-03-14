#include "optional.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

struct dummy {};

struct no_default {
  no_default() = delete;
};

struct throwing_default {
  throwing_default() {
    throw std::exception();
  }
};

struct throwing_move_operator {
  throwing_move_operator() = default;

  throwing_move_operator(throwing_move_operator&&) {
    throw std::exception();
  }

  throwing_move_operator& operator=(throwing_move_operator&&) = default;

  [[maybe_unused]] friend void swap(throwing_move_operator&, throwing_move_operator&) {
    throw std::exception();
  }
};

struct no_copy {
  no_copy(const no_copy&) = delete;
};

struct no_move_t {
  no_move_t(no_move_t&&) = delete;
};

struct non_trivial_copy {
  explicit non_trivial_copy(int x) noexcept
      : x{x} {}

  non_trivial_copy(const non_trivial_copy& other) noexcept
      : x{other.x + 1} {}

  int x;
};

struct non_trivial_copy_assignment {
  explicit non_trivial_copy_assignment(int x) noexcept
      : x{x} {}

  non_trivial_copy_assignment& operator=(const non_trivial_copy_assignment& other) {
    if (this != &other) {
      x = other.x + 5;
    }
    return *this;
  }

  int x;
};

struct no_move_assignment {
  no_move_assignment& operator=(no_move_assignment&&) = delete;
};

struct no_copy_assignment_t {
  no_copy_assignment_t& operator=(const no_copy_assignment_t&) = delete;
};

struct move_only {
  move_only(const move_only&) = delete;
  move_only(move_only&&) = default;

  move_only& operator=(const move_only&) = delete;
  move_only& operator=(move_only&&) = default;
};

struct copy_only {
  copy_only(const copy_only&) = default;
  copy_only(copy_only&&) = delete;

  copy_only& operator=(const copy_only&) = default;
  copy_only& operator=(copy_only&&) = delete;
};

struct copyable {
  copyable(const copyable&) = default;

  copyable& operator=(const copyable&) & = default;
};

struct assignable_from_int {
  explicit assignable_from_int(int) noexcept {}

  assignable_from_int& operator=(int) & noexcept {
    return *this;
  }
};

struct throwing_swappable {};

[[maybe_unused]] void swap(throwing_swappable&, throwing_swappable&) {}

struct non_swappable {};

[[maybe_unused]] void swap(non_swappable&, non_swappable&) = delete;

struct implicitly_constructible_from_any {
  template <typename T>
  constexpr implicitly_constructible_from_any(T&&) {}
};

struct swappable_no_move {
  swappable_no_move(const swappable_no_move&) = delete;
  swappable_no_move& operator=(const swappable_no_move&) = delete;
};

[[maybe_unused]] void swap(swappable_no_move&, swappable_no_move&) noexcept {}

struct swappable_no_move_assignment {
  swappable_no_move_assignment(const swappable_no_move_assignment&) = delete;
  swappable_no_move_assignment(swappable_no_move_assignment&&) = default;

  swappable_no_move_assignment& operator=(const swappable_no_move_assignment&) = delete;
  swappable_no_move_assignment& operator=(swappable_no_move_assignment&&) = delete;
};

[[maybe_unused]] void swap(swappable_no_move_assignment&, swappable_no_move_assignment&) noexcept {}

} // namespace

TEST(traits_test, types) {
  static_assert(std::is_same_v<optional<int>::value_type, int>);
  static_assert(std::is_same_v<optional<const std::string>::value_type, const std::string>);
}

TEST(traits_test, nullopt) {
  static_assert(std::is_same_v<decltype(nullopt), const nullopt_t>);
  static_assert(std::is_empty_v<nullopt_t>);
  static_assert(!std::is_default_constructible_v<nullopt_t>);
}

TEST(traits_test, nullopt_ptr) {
  const nullopt_t* get_nullopt_ptr() noexcept;

  EXPECT_EQ(&nullopt, get_nullopt_ptr());
}

TEST(traits_test, in_place) {
  static_assert(std::is_same_v<decltype(in_place), const in_place_t>);
  static_assert(std::is_empty_v<in_place_t>);
  static_assert(std::is_default_constructible_v<in_place_t>);
}

TEST(traits_test, in_place_ptr) {
  const in_place_t* get_in_place_ptr() noexcept;

  EXPECT_EQ(&in_place, get_in_place_ptr());
}

TEST(traits_test, destructor) {
  static_assert(std::is_trivially_destructible_v<optional<int>>);
  static_assert(!std::is_trivially_destructible_v<optional<std::string>>);
}

TEST(traits_test, default_constructor) {
  static_assert(std::is_nothrow_default_constructible_v<optional<std::vector<int>>>);
  static_assert(std::is_nothrow_default_constructible_v<optional<no_default>>);
  static_assert(std::is_nothrow_default_constructible_v<optional<throwing_default>>);
}

TEST(traits_test, nullopt_constructor) {
  static_assert(std::is_nothrow_constructible_v<optional<std::vector<int>>, nullopt_t>);
  static_assert(std::is_nothrow_constructible_v<optional<no_default>, nullopt_t>);
  static_assert(std::is_nothrow_constructible_v<optional<throwing_default>, nullopt_t>);
}

TEST(traits_test, copy_constructor) {
  static_assert(!std::is_copy_constructible_v<optional<no_copy>>);
  static_assert(std::is_copy_constructible_v<optional<int>>);
  static_assert(std::is_nothrow_copy_constructible_v<optional<int>>);
  static_assert(std::is_copy_constructible_v<optional<std::vector<std::string>>>);
  static_assert(!std::is_nothrow_copy_constructible_v<optional<std::vector<std::string>>>);
  static_assert(!std::is_trivially_copy_constructible_v<optional<std::vector<std::string>>>);
  static_assert(std::is_trivially_copy_constructible_v<optional<dummy>>);
  static_assert(!std::is_trivially_copy_constructible_v<optional<non_trivial_copy>>);
  static_assert(std::is_trivially_copy_constructible_v<optional<copy_only>>);
}

TEST(traits_test, move_constructor) {
  static_assert(!std::is_move_constructible_v<optional<no_move_t>>);
  static_assert(std::is_move_constructible_v<optional<std::string>>);
  static_assert(std::is_nothrow_move_constructible_v<optional<std::string>>);
  static_assert(!std::is_trivially_move_constructible_v<optional<std::string>>);
  static_assert(std::is_move_constructible_v<optional<dummy>>);
  static_assert(std::is_trivially_move_constructible_v<optional<dummy>>);
  static_assert(std::is_move_constructible_v<optional<throwing_move_operator>>);
  static_assert(std::is_trivially_move_constructible_v<optional<move_only>>);
}

TEST(traits_test, copy_assignment) {
  static_assert(!std::is_copy_assignable_v<optional<no_copy>>);
  static_assert(!std::is_copy_assignable_v<optional<no_copy_assignment_t>>);
  static_assert(std::is_copy_assignable_v<optional<non_trivial_copy_assignment>>);
  static_assert(std::is_copy_assignable_v<optional<non_trivial_copy>>);
  static_assert(std::is_copy_assignable_v<optional<dummy>>);
  static_assert(std::is_nothrow_copy_assignable_v<optional<dummy>>);
  static_assert(std::is_copy_assignable_v<optional<std::string>>);
  static_assert(!std::is_nothrow_copy_assignable_v<optional<std::string>>);
  static_assert(!std::is_trivially_copy_assignable_v<optional<non_trivial_copy_assignment>>);
  static_assert(!std::is_trivially_copy_assignable_v<optional<non_trivial_copy>>);
  static_assert(std::is_trivially_copy_assignable_v<optional<dummy>>);
  static_assert(!std::is_copy_assignable_v<optional<no_copy>>);
}

TEST(traits_test, move_assignment) {
  static_assert(!std::is_move_assignable_v<optional<no_move_t>>);
  static_assert(!std::is_move_assignable_v<optional<no_move_assignment>>);
  static_assert(std::is_move_assignable_v<optional<std::vector<double>>>);
  static_assert(std::is_move_assignable_v<optional<std::string>>);
  static_assert(std::is_nothrow_move_assignable_v<optional<std::string>>);
  static_assert(std::is_move_assignable_v<optional<dummy>>);
  static_assert(std::is_nothrow_move_assignable_v<optional<dummy>>);
  static_assert(!std::is_trivially_move_assignable_v<optional<std::vector<double>>>);
  static_assert(!std::is_trivially_move_assignable_v<optional<std::string>>);
  static_assert(std::is_trivially_move_assignable_v<optional<dummy>>);
  static_assert(std::is_move_assignable_v<optional<throwing_move_operator>>);
  static_assert(!std::is_move_assignable_v<optional<no_move_t>>);
}

TEST(traits_test, swap) {
  static_assert(std::is_nothrow_swappable_v<optional<dummy>>);
  static_assert(std::is_nothrow_swappable_v<optional<std::string>>);
  static_assert(std::is_nothrow_swappable_v<optional<move_only>>);
  static_assert(std::is_nothrow_swappable_v<optional<copyable>>);
  static_assert(std::is_nothrow_swappable_v<optional<swappable_no_move_assignment>>);

  static_assert(std::is_swappable_v<optional<throwing_swappable>>);
  static_assert(!std::is_nothrow_swappable_v<optional<throwing_swappable>>);

  // See https://wg21.link/LWG2766
  static_assert(!std::is_swappable_v<optional<non_swappable>>);
  static_assert(!std::is_swappable_v<optional<copy_only>>);
  static_assert(!std::is_swappable_v<optional<no_move_t>>);
  static_assert(!std::is_swappable_v<optional<swappable_no_move>>);
}

TEST(traits_test, value_constructor) {
  static_assert(std::is_constructible_v<optional<int>, int>);
  static_assert(std::is_convertible_v<int, optional<int>>);

  static_assert(std::is_constructible_v<optional<int>, long>);
  static_assert(std::is_convertible_v<long, optional<int>>);

  static_assert(std::is_constructible_v<optional<const int>, long>);
  static_assert(std::is_convertible_v<long, optional<const int>>);

  static_assert(std::is_constructible_v<optional<std::string>, const char*>);
  static_assert(std::is_convertible_v<const char*, optional<std::string>>);

  static_assert(std::is_constructible_v<optional<std::string>, std::string_view>);
  static_assert(!std::is_convertible_v<std::string_view, optional<std::string>>);

  static_assert(!std::is_constructible_v<optional<std::string>, int>);

  using opt = optional<implicitly_constructible_from_any>;
  static_assert(!std::is_convertible_v<in_place_t, opt>);
  static_assert(!std::is_convertible_v<in_place_t&, opt>);
  static_assert(!std::is_convertible_v<const in_place_t&, opt>);
  static_assert(!std::is_convertible_v<in_place_t&&, opt>);
  static_assert(!std::is_convertible_v<const in_place_t&&, opt>);
}

TEST(traits_test, value_assignment) {
  static_assert(std::is_nothrow_assignable_v<optional<int>&, int>);
  static_assert(!std::is_assignable_v<const optional<int>&, int>);

  static_assert(std::is_nothrow_assignable_v<optional<int>&, long>);
  static_assert(!std::is_assignable_v<const optional<int>&, long>);

  static_assert(!std::is_nothrow_assignable_v<optional<const int>&, long>);
  static_assert(!std::is_assignable_v<const optional<const int>&, long>);

  static_assert(std::is_assignable_v<optional<std::string>&, const char*>);
  static_assert(!std::is_nothrow_assignable_v<optional<std::string>&, const char*>);
  static_assert(!std::is_assignable_v<const optional<std::string>&, const char*>);

  static_assert(std::is_assignable_v<optional<std::string>&, std::string_view>);
  static_assert(!std::is_nothrow_assignable_v<optional<std::string>&, std::string_view>);
  static_assert(!std::is_assignable_v<const optional<std::string>&, std::string_view>);

  static_assert(!std::is_assignable_v<optional<std::string>&, int>);
  static_assert(!std::is_assignable_v<const optional<std::string>&, int>);

  static_assert(std::is_nothrow_assignable_v<optional<assignable_from_int>&, int>);
  static_assert(!std::is_assignable_v<const optional<assignable_from_int>&, int>);
}

namespace {

template <bool B>
struct conditionally_noexcept_1 {
  conditionally_noexcept_1() noexcept(B);
  conditionally_noexcept_1(const conditionally_noexcept_1&) noexcept(B);

  conditionally_noexcept_1(int) noexcept(B);

  conditionally_noexcept_1(const conditionally_noexcept_1<!B>&) noexcept(B);

  conditionally_noexcept_1& operator=(const conditionally_noexcept_1&) noexcept(false);
};

template <bool B>
struct conditionally_noexcept_2 {
  conditionally_noexcept_2() noexcept(B);

  conditionally_noexcept_2(const conditionally_noexcept_2&) noexcept(B);
  conditionally_noexcept_2& operator=(const conditionally_noexcept_2&) noexcept(B);

  conditionally_noexcept_2(int) noexcept(B);
  conditionally_noexcept_2& operator=(int) noexcept(false);

  conditionally_noexcept_2(void*) noexcept(true);
  conditionally_noexcept_2& operator=(void*) noexcept(B);

  conditionally_noexcept_2(const conditionally_noexcept_2<!B>&) noexcept(B);
  conditionally_noexcept_2& operator=(const conditionally_noexcept_2<!B>&) noexcept(B);
};

} // namespace

TEST(traits_test, noexcept_constructor) {
  using X = conditionally_noexcept_1<true>;
  using Y = conditionally_noexcept_1<false>;
  using OX = optional<X>;
  using OY = optional<Y>;

  static_assert(std::is_nothrow_constructible_v<OX>);
  static_assert(std::is_nothrow_constructible_v<OX, nullopt_t>);
  static_assert(std::is_nothrow_constructible_v<OX, const X&>);
  static_assert(std::is_nothrow_constructible_v<OX, X>);
  static_assert(std::is_nothrow_constructible_v<OX, in_place_t, short>);
  static_assert(std::is_nothrow_constructible_v<OX, const Y&>);
  static_assert(std::is_nothrow_constructible_v<OX, Y>);

  static_assert(std::is_nothrow_constructible_v<OY>);
  static_assert(std::is_nothrow_constructible_v<OY, nullopt_t>);
  static_assert(!std::is_nothrow_constructible_v<OY, const Y&>);
  static_assert(!std::is_nothrow_constructible_v<OY, Y>);
  static_assert(!std::is_nothrow_constructible_v<OY, in_place_t, short>);
  static_assert(!std::is_nothrow_constructible_v<OY, const X&>);
  static_assert(!std::is_nothrow_constructible_v<OY, X>);
}

TEST(traits_test, noexcept_assignment) {
  using X = conditionally_noexcept_2<true>;
  using Y = conditionally_noexcept_2<false>;
  using OX = optional<X>;
  using OY = optional<Y>;

  static_assert(std::is_nothrow_assignable_v<OX, nullopt_t>);
  static_assert(std::is_nothrow_assignable_v<OX, const X&>);
  static_assert(std::is_nothrow_assignable_v<OX, X>);
  static_assert(!std::is_nothrow_assignable_v<OX, int>);
  static_assert(std::is_nothrow_assignable_v<OX, void*>);
  static_assert(std::is_nothrow_assignable_v<OX, const Y&>);
  static_assert(std::is_nothrow_assignable_v<OX, Y>);

  static_assert(std::is_nothrow_assignable_v<OY, nullopt_t>);
  static_assert(!std::is_nothrow_assignable_v<OY, const Y&>);
  static_assert(!std::is_nothrow_assignable_v<OY, Y>);
  static_assert(!std::is_nothrow_assignable_v<OY, int>);
  static_assert(!std::is_nothrow_assignable_v<OY, void*>);
  static_assert(!std::is_nothrow_assignable_v<OY, const X&>);
  static_assert(!std::is_nothrow_assignable_v<OY, X>);

  static_assert(noexcept(std::declval<OX&>().emplace()));
  static_assert(noexcept(std::declval<OX&>().emplace(std::declval<X&>())));
  static_assert(noexcept(std::declval<OX&>().emplace(1)));
  static_assert(noexcept(std::declval<OX&>().emplace(nullptr)));
  static_assert(noexcept(std::declval<OX&>().emplace(std::declval<Y&>())));

  static_assert(!noexcept(std::declval<OY&>().emplace()));
  static_assert(!noexcept(std::declval<OY&>().emplace(std::declval<Y&>())));
  static_assert(!noexcept(std::declval<OY&>().emplace(1)));
  static_assert(noexcept(std::declval<OY&>().emplace(nullptr)));
  static_assert(!noexcept(std::declval<OY&>().emplace(std::declval<X&>())));
}

TEST(traits_test, lwg2762) {
  struct S {
    void can_throw();
    void cannot_throw() noexcept;
  };

  static_assert(!noexcept(std::declval<optional<S>&>()->can_throw()));
  static_assert(noexcept(std::declval<optional<S>&>()->cannot_throw()));

  static_assert(noexcept(std::declval<optional<S>&>().operator->()));
  static_assert(noexcept(std::declval<optional<int>&>().operator->()));

  static_assert(noexcept(*std::declval<optional<int>&>()));
  static_assert(noexcept(*std::declval<const optional<int>&>()));
  static_assert(noexcept(*std::declval<optional<int>&&>()));
  static_assert(noexcept(*std::declval<const optional<int>&&>()));
}

namespace {

enum class variant {
  TRIVIAL,
  DELETED,
  USER_DEFINED,
  USER_DEFINED_NOEXCEPT,
};

constexpr bool is_variant_noexcept(variant v) noexcept {
  return v == variant::USER_DEFINED_NOEXCEPT || v == variant::TRIVIAL;
}

template <variant...>
struct variants {};

template <variant V>
consteval std::string_view to_str() noexcept {
  if constexpr (V == variant::TRIVIAL) {
    return "trivial";
  } else if constexpr (V == variant::DELETED) {
    return "deleted";
  } else if constexpr (V == variant::USER_DEFINED) {
    return "user-defined";
  } else if constexpr (V == variant::USER_DEFINED_NOEXCEPT) {
    return "user-defined (noexcept)";
  }
}

template <variant Dtor, variant CopyCtor, variant MoveCtor, variant CopyAssign, variant MoveAssign>
struct test_variants_error {
  friend std::ostream& operator<<(std::ostream& out, test_variants_error) {
    out << "Dtor = " << to_str<Dtor>() << ", ";
    out << "CopyCtor = " << to_str<CopyCtor>() << ", ";
    out << "MoveCtor = " << to_str<MoveCtor>() << ", ";
    out << "CopyAssign = " << to_str<CopyAssign>() << ", ";
    out << "MoveAssign = " << to_str<MoveAssign>();
    return out;
  }
};

// workaround for clang 16+
template <variant Dtor>
struct test_object_destructor_base;

template <>
struct test_object_destructor_base<variant::TRIVIAL> {};

template <>
struct test_object_destructor_base<variant::USER_DEFINED_NOEXCEPT> {
  ~test_object_destructor_base() noexcept {}
};

template <variant Dtor, variant CopyCtor, variant MoveCtor, variant CopyAssign, variant MoveAssign>
void test_variants() {
  using enum variant;

  struct test_object : test_object_destructor_base<Dtor> {
    test_object(const test_object&)
      requires (CopyCtor == TRIVIAL)
    = default;

    test_object(const test_object&)
      requires (CopyCtor == DELETED)
    = delete;

    test_object(const test_object&)
      requires (CopyCtor == USER_DEFINED)
    {}

    test_object(const test_object&) noexcept
      requires (CopyCtor == USER_DEFINED_NOEXCEPT)
    {}

    test_object(test_object&&)
      requires (MoveCtor == TRIVIAL)
    = default;

    test_object(test_object&&)
      requires (MoveCtor == DELETED)
    = delete;

    test_object(test_object&&)
      requires (MoveCtor == USER_DEFINED)
    {}

    test_object(test_object&&) noexcept
      requires (MoveCtor == USER_DEFINED_NOEXCEPT)
    {}

    test_object& operator=(const test_object&)
      requires (CopyAssign == TRIVIAL)
    = default;

    test_object& operator=(const test_object&)
      requires (CopyAssign == DELETED)
    = delete;

    test_object& operator=(const test_object&)
      requires (CopyAssign == USER_DEFINED)
    {
      return *this;
    }

    test_object& operator=(const test_object&) noexcept
      requires (CopyAssign == USER_DEFINED_NOEXCEPT)
    {
      return *this;
    }

    test_object& operator=(test_object&&)
      requires (MoveAssign == TRIVIAL)
    = default;

    test_object& operator=(test_object&&)
      requires (MoveAssign == DELETED)
    = delete;

    test_object& operator=(test_object&&)
      requires (MoveAssign == USER_DEFINED)
    {
      return *this;
    }

    test_object& operator=(test_object&&) noexcept
      requires (MoveAssign == USER_DEFINED_NOEXCEPT)
    {
      return *this;
    }
  };

  test_variants_error<Dtor, CopyCtor, MoveCtor, CopyAssign, MoveAssign> err;

  {
    using opt = optional<test_object>;

    if constexpr (CopyCtor != DELETED) {
      EXPECT_TRUE(std::is_copy_constructible_v<opt>) << err;
    }
    if constexpr (MoveCtor != DELETED) {
      EXPECT_TRUE(std::is_move_constructible_v<opt>) << err;
    }
    if constexpr (CopyAssign != DELETED && CopyCtor != DELETED) {
      EXPECT_TRUE(std::is_copy_assignable_v<opt>) << err;
    }
    if constexpr (MoveAssign != DELETED && MoveCtor != DELETED) {
      EXPECT_TRUE(std::is_move_assignable_v<opt>) << err;
    }
  }

  {
    // inherit so that deleted moves still allow for calling copy ctor/assignment
    struct derived : test_object {
      using test_object::test_object;
    };

    using opt = optional<derived>;

    EXPECT_EQ(std::is_destructible_v<derived>, std::is_destructible_v<opt>) << err;
    EXPECT_EQ(std::is_nothrow_destructible_v<derived>, std::is_nothrow_destructible_v<opt>) << err;
    EXPECT_EQ(std::is_trivially_destructible_v<derived>, std::is_trivially_destructible_v<opt>) << err;

    EXPECT_EQ(std::is_copy_constructible_v<derived>, std::is_copy_constructible_v<opt>) << err;
    EXPECT_EQ(std::is_nothrow_copy_constructible_v<derived>, std::is_nothrow_copy_constructible_v<opt>) << err;
    EXPECT_EQ(std::is_trivially_copy_constructible_v<derived>, std::is_trivially_copy_constructible_v<opt>) << err;

    EXPECT_EQ(std::is_move_constructible_v<derived>, std::is_move_constructible_v<opt>) << err;
    EXPECT_EQ(std::is_nothrow_move_constructible_v<derived>, std::is_nothrow_move_constructible_v<opt>) << err;
    EXPECT_EQ(std::is_trivially_move_constructible_v<derived>, std::is_trivially_move_constructible_v<opt>) << err;

    if constexpr (CopyCtor == DELETED) {
      EXPECT_FALSE(std::is_copy_assignable_v<opt>) << err;
    } else {
      EXPECT_EQ(std::is_copy_assignable_v<derived>, std::is_copy_assignable_v<opt>) << err;
    }

    if constexpr (is_variant_noexcept(CopyCtor)) {
      EXPECT_EQ(std::is_nothrow_copy_assignable_v<derived>, std::is_nothrow_copy_assignable_v<opt>) << err;
    } else {
      EXPECT_FALSE(std::is_nothrow_copy_assignable_v<opt>) << err;
    }

    if constexpr (CopyCtor == TRIVIAL && Dtor == TRIVIAL) {
      EXPECT_EQ(std::is_trivially_copy_assignable_v<derived>, std::is_trivially_copy_assignable_v<opt>) << err;
    } else {
      EXPECT_FALSE(std::is_trivially_copy_assignable_v<opt>) << err;
    }

    if constexpr (MoveCtor == DELETED && CopyCtor == DELETED) {
      EXPECT_FALSE(std::is_move_assignable_v<opt>) << err;
    } else {
      EXPECT_EQ(std::is_move_assignable_v<derived>, std::is_move_assignable_v<opt>) << err;
    }

    if constexpr (is_variant_noexcept(MoveCtor) || (MoveCtor == DELETED && is_variant_noexcept(CopyCtor))) {
      EXPECT_EQ(std::is_nothrow_move_assignable_v<derived>, std::is_nothrow_move_assignable_v<opt>) << err;
    } else {
      EXPECT_FALSE(std::is_nothrow_move_assignable_v<opt>) << err;
    }

    if constexpr ((MoveCtor == TRIVIAL || (MoveCtor == DELETED && CopyCtor == TRIVIAL)) && Dtor == TRIVIAL) {
      EXPECT_EQ(std::is_trivially_move_assignable_v<derived>, std::is_trivially_move_assignable_v<opt>) << err;
    } else {
      EXPECT_FALSE(std::is_trivially_move_assignable_v<opt>) << err;
    }
  }
}

template <variant... Vs, typename F>
void static_for_each(variants<Vs...>, F f) {
  (f.template operator()<Vs>(), ...);
}

} // namespace

TEST(traits_test, all_variants) {
  using dtor_variants = variants<variant::TRIVIAL, variant::USER_DEFINED_NOEXCEPT>;
  using all_variants =
      variants<variant::TRIVIAL, variant::DELETED, variant::USER_DEFINED, variant::USER_DEFINED_NOEXCEPT>;

  static_for_each(dtor_variants{}, []<variant Dtor>() {
    static_for_each(all_variants{}, []<variant CopyCtor>() {
      static_for_each(all_variants{}, []<variant MoveCtor>() {
        static_for_each(all_variants{}, []<variant CopyAssign>() {
          static_for_each(all_variants{}, []<variant MoveAssign>() {
            test_variants<Dtor, CopyCtor, MoveCtor, CopyAssign, MoveAssign>();
          });
        });
      });
    });
  });
}
