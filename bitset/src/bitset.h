#pragma once

#include "bitset-constants.h"
#include "bitset-iterator.h"
#include "bitset-view.h"

#include <cstddef>
#include <limits>
#include <string_view>

class bitset {
public:
  using value_type = bool;
  using word_type = unsigned long long;
  using reference = bitset_reference<word_type>;
  using const_reference = bitset_reference<const word_type>;
  using iterator = bitset_iterator<word_type>;
  using const_iterator = bitset_iterator<const word_type>;
  using view = bitset_view<word_type>;
  using const_view = bitset_view<const word_type>;

  static constexpr std::size_t npos = -1;

  bitset();
  bitset(std::size_t size, bool value);
  bitset(const bitset& other);
  explicit bitset(std::string_view str);
  explicit bitset(const const_view& other);
  bitset(const_iterator first, const_iterator last);

  bitset& operator=(const bitset& other) &;
  bitset& operator=(std::string_view str) &;
  bitset& operator=(const const_view& other) &;

  ~bitset();

  void swap(bitset& other);

  std::size_t size() const;
  bool empty() const;

  reference operator[](std::size_t index);
  const_reference operator[](std::size_t index) const;

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  bitset& operator&=(const const_view& other) &;
  bitset& operator|=(const const_view& other) &;
  bitset& operator^=(const const_view& other) &;
  bitset& operator<<=(std::size_t count) &;
  bitset& operator>>=(std::size_t count) &;
  bitset& flip() &;

  bitset& set() &;
  bitset& reset() &;

  bool all() const;
  bool any() const;
  std::size_t count() const;

  operator const_view() const;
  operator view();

  view subview(std::size_t offset = 0, std::size_t count = npos);
  const_view subview(std::size_t offset = 0, std::size_t count = npos) const;

private:
  bitset& iteration_for_shifts(std::size_t count, std::size_t size);
  bitset(std::size_t size);

  word_type* data_;
  std::size_t size_;
};

bitset operator&(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator|(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator^(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator<<(const bitset::const_view& vw, std::size_t count);
bitset operator>>(const bitset::const_view& vw, std::size_t count);
bitset operator~(const bitset::const_view& vw);

std::string to_string(const bitset::const_view& vw);
std::ostream& operator<<(std::ostream& out, const bitset::const_view& vw);

bool operator==(const bitset::const_view& left, const bitset::const_view& right);
bool operator!=(const bitset::const_view& left, const bitset::const_view& right);

void swap(bitset& lhs, bitset& rhs);
