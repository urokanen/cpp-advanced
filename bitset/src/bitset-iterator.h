#pragma once

#include "bitset-constants.h"
#include "bitset-reference.h"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <numeric>
#include <type_traits>

template <typename T>
class bitset_iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = bool;
  using reference = bitset_reference<T>;
  using const_reference = bitset_reference<const T>;
  using pointer = void;

  bitset_iterator() = default;

  bitset_iterator(const bitset_iterator& other) = default;

  bitset_iterator& operator=(const bitset_iterator& other) = default;

  operator bitset_iterator<const T>() const {
    return {data_, index_};
  }

  reference operator*() const {
    return reference(data_[index_ / constants::word_size_bits], index_ % constants::word_size_bits);
  }

  bitset_iterator& operator++() {
    index_++;
    return *this;
  }

  bitset_iterator operator++(int) {
    bitset_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  bitset_iterator& operator--() {
    index_--;
    return *this;
  }

  bitset_iterator operator--(int) {
    bitset_iterator tmp = *this;
    --*this;
    return tmp;
  }

  bitset_iterator& operator+=(difference_type n) {
    index_ += static_cast<std::size_t>(n);
    return *this;
  }

  bitset_iterator& operator-=(difference_type n) {
    index_ -= static_cast<std::size_t>(n);
    return *this;
  }

  bitset_iterator operator+(difference_type n) const {
    bitset_iterator tmp = *this;
    tmp.index_ += static_cast<std::size_t>(n);
    return tmp;
  }

  friend bitset_iterator operator+(difference_type n, const bitset_iterator& it) {
    return it + n;
  }

  bitset_iterator operator-(difference_type n) const {
    bitset_iterator tmp = *this;
    tmp.index_ -= static_cast<std::size_t>(n);
    return tmp;
  }

  friend difference_type operator-(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return static_cast<difference_type>(lhs.index_ - rhs.index_);
  }

  reference operator[](difference_type n) const {
    return *(*this + n);
  }

  friend bool operator==(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return lhs.index_ == rhs.index_;
  }

  friend bool operator!=(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<=(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return lhs.index_ <= rhs.index_;
  }

  friend bool operator<(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return lhs.index_ < rhs.index_;
  }

  friend bool operator>=(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return lhs.index_ >= rhs.index_;
  }

  friend bool operator>(const bitset_iterator& lhs, const bitset_iterator& rhs) {
    return lhs.index_ > rhs.index_;
  }

  ~bitset_iterator() = default;

private:
  T get(std::size_t size) const {
    std::size_t shift = index_ % constants::word_size_bits;
    if (size <= constants::word_size_bits - shift) {
      return (
          ((data_[index_ / constants::word_size_bits] >> (shift)) << (constants::word_size_bits - size)) >>
          (constants::word_size_bits - size)
      );
    }
    std::size_t remainder = size - constants::word_size_bits + shift;
    std::size_t delta = constants::word_size_bits - remainder;
    return (
        (data_[index_ / constants::word_size_bits] >> (shift)) ^
        (((data_[index_ / constants::word_size_bits + 1] << delta) >> delta) << (constants::word_size_bits - shift))
    );
  }

  void set(T value, std::size_t size) const
    requires (!std::is_const_v<T>)
  {
    std::size_t shift = index_ % constants::word_size_bits;
    T mask = ((data_[index_ / constants::word_size_bits] >> shift) << shift);
    if (size < constants::word_size_bits - shift) {
      mask = ((mask << (constants::word_size_bits - shift - size)) >> (constants::word_size_bits - shift - size));
    }
    data_[index_ / constants::word_size_bits] ^= mask;
    data_[index_ / constants::word_size_bits] |= (value << shift);
    if (constants::word_size_bits - shift < size) {
      std::size_t delta = static_cast<std::size_t>(2) * constants::word_size_bits - size - shift;
      mask = ((data_[index_ / constants::word_size_bits + 1] << delta) >> delta);
      data_[index_ / constants::word_size_bits + 1] ^= mask;
      data_[index_ / constants::word_size_bits + 1] |= (value >> (constants::word_size_bits - shift));
    }
  }

  bitset_iterator(T* data, std::size_t index)
      : data_(data)
      , index_(index) {}

  friend class bitset;
  template <typename T1>
  friend class bitset_view;
  template <typename T2>
  friend class bitset_iterator;

  T* data_;
  std::size_t index_;
};
