#pragma once

#include <cstddef>
#include <limits>

namespace constants {
using word_type = unsigned long long;
static constexpr std::size_t word_size_bits = std::numeric_limits<word_type>::digits;
static constexpr word_type max = -1;
static constexpr word_type one = 1;
static constexpr std::size_t npos = -1;
} // namespace constants
