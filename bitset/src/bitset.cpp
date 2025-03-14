#include "bitset.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <ostream>
#include <string>

bitset::bitset()
    : data_(nullptr)
    , size_(0) {}

bitset::bitset(std::size_t size)
    : size_(size) {
  if (size_ > 0) {
    data_ = new word_type[(size_ + constants::word_size_bits - 1) / constants::word_size_bits]();
  } else {
    data_ = nullptr;
  }
}

bitset::bitset(std::size_t size, bool value)
    : bitset(size) {
  if (value) {
    std::fill_n(data_, size_ / constants::word_size_bits, constants::max);
    if (size_ % constants::word_size_bits != 0) {
      auto val = constants::one;
      val <<= (size_ % constants::word_size_bits);
      val--;
      data_[size_ / constants::word_size_bits] = val;
    }
  }
}

bitset::bitset(const bitset& other)
    : bitset(other.size_) {
  std::size_t count = (size_ + constants::word_size_bits - 1) / constants::word_size_bits;
  std::copy(other.data_, other.data_ + count, data_);
}

bitset::bitset(std::string_view str)
    : bitset(str.size()) {
  std::size_t count = 0;
  for (auto c : str) {
    if (c == '1') {
      data_[count / constants::word_size_bits] ^= (constants::one) << (count % constants::word_size_bits);
    }
    count++;
  }
}

bitset::bitset(const const_view& other)
    : bitset(other.begin(), other.end()) {}

bitset::bitset(const_iterator first, const_iterator last)
    : bitset(static_cast<std::size_t>(last - first)) {
  const_iterator it = first;
  while (it < last) {
    std::size_t count = std::min(constants::word_size_bits, static_cast<std::size_t>(last - it));
    data_[static_cast<std::size_t>(it - first) / constants::word_size_bits] = it.get(count);
    it += static_cast<ptrdiff_t>(count);
  }
}

bitset& bitset::operator=(const bitset& other) & {
  if (&other != this) {
    bitset(other).swap(*this);
  }
  return *this;
}

bitset& bitset::operator=(std::string_view str) & {
  bitset copy = bitset(str);
  swap(copy);
  return *this;
}

bitset& bitset::operator=(const const_view& other) & {
  bitset copy = bitset(other);
  swap(copy);
  return *this;
}

bitset::~bitset() {
  delete[] data_;
}

void bitset::swap(bitset& other) {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
}

std::size_t bitset::size() const {
  return size_;
}

bool bitset::empty() const {
  return size() == 0;
}

bitset::reference bitset::operator[](std::size_t index) {
  return {data_[index / constants::word_size_bits], index % constants::word_size_bits};
}

bitset::const_reference bitset::operator[](std::size_t index) const {
  return {data_[index / constants::word_size_bits], index % constants::word_size_bits};
}

bitset::iterator bitset::begin() {
  return {data_, 0};
}

bitset::const_iterator bitset::begin() const {
  return {data_, 0};
}

bitset::iterator bitset::end() {
  return {data_, size()};
}

bitset::const_iterator bitset::end() const {
  return {data_, size()};
}

bitset& bitset::operator&=(const const_view& other) & {
  view vw = subview();
  vw &= other;
  return *this;
}

bitset& bitset::operator|=(const const_view& other) & {
  view vw = subview();
  vw |= other;
  return *this;
}

bitset& bitset::operator^=(const const_view& other) & {
  view vw = subview();
  vw ^= other;
  return *this;
}

bitset& bitset::iteration_for_shifts(std::size_t count, std::size_t size) {
  bitset copy = bitset(count, false);
  std::size_t ind = 0;
  while (ind < size) {
    if (ind + constants::word_size_bits <= size) {
      copy.data_[ind / constants::word_size_bits] = data_[ind / constants::word_size_bits];
      ind += constants::word_size_bits;
    } else {
      copy.data_[ind / constants::word_size_bits] ^=
          (data_[ind / constants::word_size_bits] & (constants::one << (ind % constants::word_size_bits)));
      ind++;
    }
  }
  swap(copy);
  return *this;
}

bitset& bitset::operator<<=(std::size_t count) & {
  return iteration_for_shifts(size() + count, size());
}

bitset& bitset::operator>>=(std::size_t count) & {
  if (size() >= count) {
    return iteration_for_shifts(size() - count, size() - count);
  }
  bitset copy = bitset();
  swap(copy);
  return *this;
}

bitset& bitset::flip() & {
  view vw = subview();
  vw.flip();
  return *this;
}

bitset& bitset::set() & {
  view vw = subview();
  vw.set();
  return *this;
}

bitset& bitset::reset() & {
  view vw = subview();
  vw.reset();
  return *this;
}

bool bitset::all() const {
  return subview().all();
}

bool bitset::any() const {
  return subview().any();
}

std::size_t bitset::count() const {
  std::size_t ans = 0;
  for (std::size_t ind = 0; ind < size(); ind += constants::word_size_bits) {
    ans += std::popcount(data_[ind / constants::word_size_bits]);
  }
  return ans;
}

bitset::operator const_view() const {
  return {begin(), end()};
}

bitset::operator view() {
  return {begin(), end()};
}

bitset::view bitset::subview(std::size_t offset, std::size_t count) {
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

bitset::const_view bitset::subview(std::size_t offset, std::size_t count) const {
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

bitset operator&(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset ans = bitset(lhs);
  ans &= rhs;
  return ans;
}

bitset operator|(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset ans = bitset(lhs);
  ans |= rhs;
  return ans;
}

bitset operator^(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset ans = bitset(lhs);
  ans ^= rhs;
  return ans;
}

bitset operator~(const bitset::const_view& vw) {
  bitset ans = bitset(vw);
  ans.flip();
  return ans;
}

bitset operator<<(const bitset::const_view& vw, std::size_t count) {
  bitset ans = bitset(vw);
  ans <<= count;
  return ans;
}

bitset operator>>(const bitset::const_view& vw, std::size_t count) {
  bitset ans = bitset(vw);
  ans >>= count;
  return ans;
}

std::string to_string(const bitset::const_view& vw) {
  std::string ans;
  ans.reserve(vw.size());
  for (auto el : vw) {
    ans += el ? '1' : '0';
  }
  return ans;
}

std::ostream& operator<<(std::ostream& out, const bitset::const_view& vw) {
  for (auto el : vw) {
    out << (el ? "1" : "0");
  }
  return out;
}

void swap(bitset& lhs, bitset& rhs) {
  lhs.swap(rhs);
}
