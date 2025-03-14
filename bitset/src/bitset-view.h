#pragma once

#include "bitset-constants.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>

template <typename T>
class bitset_view {
public:
  using value_type = bool;
  using word_type = T;
  using reference = bitset_reference<word_type>;
  using const_reference = bitset_reference<const word_type>;
  using iterator = bitset_iterator<word_type>;
  using const_iterator = bitset_iterator<const word_type>;
  using view = bitset_view<word_type>;
  using const_view = bitset_view<const word_type>;

  bitset_view() = default;

  bitset_view(iterator left, iterator right)
      : left_(left)
      , right_(right) {}

  bitset_view(const bitset_view& other) = default;

  bitset_view& operator=(const bitset_view& other) = default;

  operator const_view() const {
    return {left_, right_};
  }

  void swap(view& other) {
    std::swap(left_, other.left_);
    std::swap(right_, other.right_);
  }

  std::size_t size() const {
    return static_cast<std::size_t>(right_ - left_);
  }

  bool empty() const {
    return size() == 0;
  }

  reference operator[](std::size_t index) const {
    return *(begin() + static_cast<ptrdiff_t>(index));
  }

  iterator begin() const {
    return left_;
  }

  iterator end() const {
    return right_;
  }

  view operator&=(const const_view& other) const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_bits_operation(std::bit_and<word_type>{}, other);
  }

  view operator|=(const const_view& other) const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_bits_operation(std::bit_or<word_type>{}, other);
  }

  view operator^=(const const_view& other) const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_bits_operation(std::bit_xor<word_type>{}, other);
  }

  view flip() const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_operation([](word_type value, std::size_t count) {
      return value ^ (constants::max >> (constants::word_size_bits - count));
    });
  }

  view set() const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_operation([]([[maybe_unused]] word_type value, std::size_t count) {
      return (constants::max >> (constants::word_size_bits - count));
    });
  }

  view reset() const
    requires (!std::is_const_v<T>)
  {
    return iteration_with_operation([]([[maybe_unused]] word_type value, [[maybe_unused]] std::size_t count) {
      return static_cast<word_type>(0);
    });
  }

  bool all() const {
    return iteration_for_bool(
        [](word_type value, std::size_t count) {
          return !(value == (constants::max >> (constants::word_size_bits - count)));
        },
        false
    );
  }

  bool any() const {
    return iteration_for_bool([](word_type value, [[maybe_unused]] std::size_t count) { return value; }, true);
  }

  std::size_t count() const {
    std::size_t ans = 0;
    iterator it = begin();
    while (it < end()) {
      std::size_t count = std::min(constants::word_size_bits, static_cast<std::size_t>(end() - it));
      word_type value = it.get(count);
      ans += std::popcount(value);
      it += static_cast<ptrdiff_t>(count);
    }
    return ans;
  }

  view subview(std::size_t offset = 0, std::size_t count = constants::npos) {
    if (offset > size()) {
      return {end(), end()};
    } else if (count <= size() - offset) {
      return {
          begin() + static_cast<ptrdiff_t>(offset),
          begin() + static_cast<ptrdiff_t>(offset) + static_cast<ptrdiff_t>(count)
      };
    } else {
      return {begin() + static_cast<ptrdiff_t>(offset), end()};
    }
  }

  const_view subview(std::size_t offset = 0, std::size_t count = constants::npos) const {
    if (offset > size()) {
      return {end(), end()};
    } else if (count <= size() - offset) {
      return {
          begin() + static_cast<ptrdiff_t>(offset),
          begin() + static_cast<ptrdiff_t>(offset) + static_cast<ptrdiff_t>(count)
      };
    } else {
      return {begin() + static_cast<ptrdiff_t>(offset), end()};
    }
  }

  friend bool operator==(const view& left, const view& right) {
    if (left.size() == right.size()) {
      std::size_t ind = 0;
      while (ind < left.size()) {
        std::size_t count = std::min(constants::word_size_bits, left.size() - ind);
        if (left.get_word(ind, count) != right.get_word(ind, count)) {
          return false;
        }
        ind += count;
      }
      return true;
    }
    return false;
  }

  friend bool operator!=(const view& left, const view& right) {
    return !(left == right);
  }

  ~bitset_view() = default;

private:
  template <typename Func>
  bool iteration_for_bool(const Func func, bool cond_result) const {
    iterator it = begin();
    while (it < end()) {
      std::size_t count = std::min(constants::word_size_bits, static_cast<std::size_t>(end() - it));
      if (func(it.get(count), count)) {
        return cond_result;
      }
      it += static_cast<ptrdiff_t>(count);
    }
    return !cond_result;
  }

  template <typename Func>
  view iteration_with_operation(const Func func) const {
    iterator it = begin();
    while (it < end()) {
      std::size_t count = std::min(constants::word_size_bits, static_cast<std::size_t>(end() - it));
      it.set(func(it.get(count), count), count);
      it += static_cast<ptrdiff_t>(count);
    }
    return view(*this);
  }

  template <typename Func>
  view iteration_with_bits_operation(const Func bit_function, const const_view& other) const {
    iterator it = begin();
    while (it < end()) {
      std::size_t count = std::min(constants::word_size_bits, static_cast<std::size_t>(end() - it));
      const_iterator it2 = other.begin() + static_cast<ptrdiff_t>(it - begin());
      it.set(bit_function(it.get(count), it2.get(count)), count);
      it += static_cast<ptrdiff_t>(count);
    }
    return view(*this);
  }

  word_type get_word(std::size_t pos, std::size_t count) const {
    return (begin() + static_cast<ptrdiff_t>(pos)).get(count);
  }

  iterator left_, right_;
};
