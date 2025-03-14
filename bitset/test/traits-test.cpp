#include "bitset.h"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("member types") {
  SECTION("bitset") {
    STATIC_CHECK(std::is_same_v<bitset::value_type, bool>);
    STATIC_CHECK_FALSE(std::is_same_v<bitset::reference, bool>);
    STATIC_CHECK(std::numeric_limits<bitset::word_type>::digits >= 32);
  }

  SECTION("iterators") {
    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::iterator>::value_type, bitset::value_type>);
    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::const_iterator>::value_type, bitset::value_type>);

    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::iterator>::reference, bitset::reference>);
    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::const_iterator>::reference, bitset::const_reference>);

    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::iterator>::difference_type, std::ptrdiff_t>);
    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::const_iterator>::difference_type, std::ptrdiff_t>);

    STATIC_CHECK(std::is_same_v<
                 std::iterator_traits<bitset::iterator>::iterator_category,
                 std::random_access_iterator_tag>);
    STATIC_CHECK(std::is_same_v<
                 std::iterator_traits<bitset::const_iterator>::iterator_category,
                 std::random_access_iterator_tag>);

    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::iterator>::pointer, void>);
    STATIC_CHECK(std::is_same_v<std::iterator_traits<bitset::const_iterator>::pointer, void>);
  }

  SECTION("views") {
    STATIC_CHECK(std::is_same_v<bitset::view::value_type, bitset::value_type>);
    STATIC_CHECK(std::is_same_v<bitset::const_view::value_type, bitset::value_type>);

    STATIC_CHECK(std::is_same_v<bitset::view::reference, bitset::reference>);
    STATIC_CHECK(std::is_same_v<bitset::view::const_reference, bitset::const_reference>);
    STATIC_CHECK(std::is_same_v<bitset::const_view::reference, bitset::const_reference>);
    STATIC_CHECK(std::is_same_v<bitset::const_view::const_reference, bitset::const_reference>);
  }
}
