#pragma once

#include <cstddef>
#include <type_traits>

template <typename T>
class bitset_reference {
public:
  bitset_reference() = delete;

  bitset_reference(const bitset_reference& other) = default;

  bitset_reference& operator=(const bitset_reference& other) = default;

  operator bitset_reference<const T>() const {
    return {element_, index_};
  }

  operator bool() const {
    return static_cast<bool>((element_ >> index_) & static_cast<T>(1));
  }

  bitset_reference& operator=(bool val)
    requires (!std::is_const_v<T>)
  {
    if (*this != val) {
      flip();
    }
    return *this;
  }

  bitset_reference& flip()
    requires (!std::is_const_v<T>)
  {
    element_ ^= (static_cast<T>(1) << index_);
    return *this;
  }

  ~bitset_reference() = default;

private:
  bitset_reference(T& element, std::size_t index)
      : element_(element)
      , index_(index) {}

  friend class bitset;
  template <typename T1>
  friend class bitset_iterator;
  template <class T2>
  friend class bitset_reference;

private:
  T& element_;
  std::size_t index_;
};
