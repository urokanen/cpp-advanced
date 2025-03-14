#include "test-utils.h"
#include "tuple.h"

#include <gtest/gtest.h>

#include <tuple>

using namespace tests;

TEST(tuple_cat, by_value) {
  tuple t1{42, 3.14};
  tuple t2{7};

  tuple cat = tuple_cat(t1, t2);
  static_assert(std::is_same_v<decltype(cat), tuple<int, double, int>>);

  EXPECT_EQ(get<0>(cat), 42);
  EXPECT_EQ(get<1>(cat), 3.14);
  EXPECT_EQ(get<2>(cat), 7);

  get<0>(t1) = 1337;
  EXPECT_EQ(get<0>(cat), 42);
}

TEST(tuple_cat, constexpr_friendly) {
  EXPECT_TRUE(([]() consteval {
    tuple t1{42, 3.14};
    tuple t2{7};

    tuple cat = tuple_cat(t1, t2);
    return get<2>(cat) == 7;
  })());
}

TEST(tuple_cat, lvalue_reference) {
  int x = 42;
  tuple<int&, double> t1{x, 3.14};
  tuple t2{7};

  tuple cat = tuple_cat(t1, t2);
  static_assert(std::is_same_v<decltype(cat), tuple<int&, double, int>>);

  EXPECT_EQ(get<0>(cat), 42);
  EXPECT_EQ(get<1>(cat), 3.14);
  EXPECT_EQ(get<2>(cat), 7);

  get<0>(t1) = 1337;
  EXPECT_EQ(get<0>(cat), 1337);
}

TEST(tuple_cat, rvalue_reference) {
  int x = 42;
  tuple<int&&, double> t1{std::move(x), 3.14};
  tuple t2{7};

  tuple cat = tuple_cat(std::move(t1), std::move(t2));
  static_assert(std::is_same_v<decltype(cat), tuple<int&&, double, int>>);

  EXPECT_EQ(get<0>(cat), 42);
  EXPECT_EQ(get<1>(cat), 3.14);
  EXPECT_EQ(get<2>(cat), 7);

  get<0>(t1) = 1337;
  EXPECT_EQ(get<0>(cat), 1337);

  get<0>(t2) = 1488;
  EXPECT_EQ(get<2>(cat), 7);
}

TEST(tuple_cat, no_args) {
  auto cat = tuple_cat();
  EXPECT_EQ(tuple_size_v<decltype(cat)>, 0);
}

TEST(tuple_cat, unary) {
  int x = 13;
  int y = 37;

  tuple<int> t1{42};
  tuple<int&> t2{x};
  tuple<int&&> t3{std::move(y)};

  auto cat1 = tuple_cat(t1);
  EXPECT_EQ(get<0>(cat1), 42);

  auto cat2 = tuple_cat(t2);
  EXPECT_EQ(get<0>(cat2), 13);

  auto cat3 = tuple_cat(std::move(t3));
  EXPECT_EQ(get<0>(cat3), 37);

  x = 14;
  y = 8;
  EXPECT_EQ(get<0>(cat2), 14);
  EXPECT_EQ(get<0>(cat3), 8);
}

TEST(tuple_cat, binary) {
  util::for_each_ref_kind([]<util::ref_kind REF_KIND>(util::constant<REF_KIND>) {
    util::combined_counter c1, c2;

    tuple t1{42, c1, true};
    tuple t2{7, 'c'};

    std::tuple s1{42, c2, true};
    std::tuple s2{7, 'c'};

    auto cat1 = tuple_cat(util::as_ref<REF_KIND>(t1), util::as_ref<REF_KIND>(t2));
    auto cat2 = std::tuple_cat(util::as_ref<REF_KIND>(s1), util::as_ref<REF_KIND>(s2));

    static_assert(tuple_size_v<decltype(cat1)> == std::tuple_size_v<decltype(cat2)>);

    util::static_for_n<tuple_size_v<decltype(cat1)>>([&](auto i) {
      ASSERT_EQ(get<i>(cat1), std::get<i>(cat2)) << "i = " << i << ", ref_kind = " << REF_KIND;
    });
  });
}

TEST(tuple_cat, ternary) {
  util::for_each_ref_kind([]<util::ref_kind REF_KIND>(util::constant<REF_KIND>) {
    util::combined_counter c1, c2;

    tuple t1{42, c1, true};
    tuple t2{false, c1, 3.14};
    tuple t3{7, 'c'};

    std::tuple s1{42, c2, true};
    std::tuple s2{false, c2, 3.14};
    std::tuple s3{7, 'c'};

    auto cat1 = tuple_cat(util::as_ref<REF_KIND>(t1), std::move(t2), util::as_ref<REF_KIND>(t3));
    auto cat2 = std::tuple_cat(util::as_ref<REF_KIND>(s1), std::move(s2), util::as_ref<REF_KIND>(s3));

    static_assert(tuple_size_v<decltype(cat1)> == std::tuple_size_v<decltype(cat2)>);

    util::static_for_n<tuple_size_v<decltype(cat1)>>([&](auto i) {
      ASSERT_EQ(get<i>(cat1), std::get<i>(cat2)) << "i = " << i << ", ref_kind = " << REF_KIND;
    });
  });
}
