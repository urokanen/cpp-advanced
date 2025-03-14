#pragma once

#include "bitset.h"

#include <catch2/matchers/catch_matchers.hpp>

#include <vector>

std::vector<bool> string_to_bools(std::string_view str);

struct bitset_equals_string : Catch::Matchers::MatcherBase<bitset> {
  explicit bitset_equals_string(std::string_view expected);

  bool match(const bitset& actual) const final;

  std::string describe() const final;

private:
  std::string_view _expected;
};
