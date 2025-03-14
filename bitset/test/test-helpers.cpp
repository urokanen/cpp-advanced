#include "test-helpers.h"

#include <ranges>

std::vector<bool> string_to_bools(std::string_view str) {
  auto view = str | std::views::transform([](char c) { return c == '1'; });
  return {view.begin(), view.end()};
}

bitset_equals_string::bitset_equals_string(std::string_view expected)
    : _expected(expected) {}

bool bitset_equals_string::match(const bitset& actual) const {
  if (actual.empty() != _expected.empty()) {
    return false;
  }
  if (actual.size() != _expected.size()) {
    return false;
  }
  for (std::size_t i = 0; i < _expected.size(); ++i) {
    bool expected_bit = (_expected[i] == '1');
    if (actual[i] != expected_bit) {
      return false;
    }
  }
  return true;
}

std::string bitset_equals_string::describe() const {
  if (_expected.empty()) {
    return "is empty";
  } else {
    return "equals " + std::string(_expected);
  }
}
