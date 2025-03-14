#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <utility>

template <typename T, std::size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  socow_vector() noexcept
      : size_(0)
      , using_small_data(true) {}

  socow_vector(const socow_vector& other)
      : size_(other.size_)
      , using_small_data(other.using_small_data) {
    if (using_small_data) {
      std::uninitialized_copy_n(other.small_data_.data(), size_, small_data_.data());
    } else {
      big_data_ = other.big_data_;
      big_data_->count++;
    }
  }

  socow_vector(socow_vector&& other) noexcept
      : size_(other.size_)
      , using_small_data(other.using_small_data) {
    if (using_small_data) {
      std::uninitialized_move_n(other.small_data_.data(), size_, small_data_.data());
      std::destroy_n(other.small_data_.data(), size_);
    } else {
      big_data_ = other.big_data_;
      other.big_data_ = nullptr;
      other.using_small_data = true;
    }
    other.size_ = 0;
  }

  socow_vector& operator=(const socow_vector& other) {
    if (this != &other) {
      socow_vector copy(other);
      full_clear();
      copy.swap(*this);
    }
    return *this;
  }

  socow_vector& operator=(socow_vector&& other) noexcept {
    if (this != &other) {
      full_clear();
      swap(other);
    }
    return *this;
  }

  void swap(socow_vector& other) noexcept {
    if (this == &other) {
      return;
    }
    if (using_small_data && other.using_small_data) {
      swap_small_data(small_data_.data(), size(), other.small_data_.data(), other.size());
    } else if (!using_small_data && !other.using_small_data) {
      std::swap(big_data_, other.big_data_);
    } else if (!using_small_data) {
      swap_big_and_small_data(other, *this);
    } else {
      swap_big_and_small_data(*this, other);
    }
    std::swap(using_small_data, other.using_small_data);
    std::swap(size_, other.size_);
  }

  ~socow_vector() noexcept {
    full_clear();
  }

  std::size_t size() const noexcept {
    return size_;
  }

  std::size_t capacity() const noexcept {
    return (using_small_data ? SMALL_SIZE : big_data_->capacity);
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  reference operator[](std::size_t index) {
    return data()[index];
  }

  const_reference operator[](std::size_t index) const noexcept {
    return data()[index];
  }

  reference front() {
    return data()[0];
  }

  const_reference front() const noexcept {
    return data()[0];
  }

  reference back() {
    return data()[size() - 1];
  }

  const_reference back() const noexcept {
    return data()[size() - 1];
  }

  pointer data() {
    check_shared();
    return current_data();
  }

  const_pointer data() const noexcept {
    return (using_small_data ? small_data_.data() : big_data_->data());
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return data() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return data() + size();
  }

  void push_back(const_reference value) {
    push_back_impl(value);
  }

  void push_back(T&& value) {
    push_back_impl(std::move(value));
  }

  iterator insert(const_iterator pos, const_reference value) {
    return insert_impl(pos, value);
  }

  iterator insert(const_iterator pos, T&& value) {
    return insert_impl(pos, std::move(value));
  }

  void pop_back() {
    if (!empty()) {
      erase(current_data() + size_ - 1, current_data() + size_);
    }
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    if (!using_small_data && big_data_->is_shared()) {
      socow_vector temp(capacity());
      auto prefix = first - current_data();
      auto suffix = last - current_data();
      temp.push_back_data(current_data(), current_data() + prefix);
      temp.push_back_data(current_data() + suffix, current_data() + size());
      swap(temp);
      return begin() + prefix;
    } else {
      auto diff = last - first;
      iterator it = iterator(last);
      for (; it < end(); it++) {
        std::iter_swap(it, it - diff);
      }
      while (diff--) {
        size_--;
        data()[size()].~value_type();
      }
      return iterator(first);
    }
  }

  void clear() {
    if (!using_small_data && big_data_->is_shared()) {
      // if we have a shared vector, we do not need to clear the data, but only need to decrease the counter
      big_data_->count--;
      big_data_ = nullptr;
      using_small_data = true;
    } else {
      std::destroy_n(data(), size_);
    }
    size_ = 0;
  }

  void reserve(std::size_t new_capacity) {
    if (size_ <= new_capacity) {
      if (using_small_data) {
        if (new_capacity > SMALL_SIZE) {
          auto* new_big_data = create_dynamic_data(new_capacity);
          std::uninitialized_move_n(small_data_.data(), size_, new_big_data->data());
          std::destroy_n(small_data_.data(), size_);
          big_data_ = new_big_data;
          using_small_data = false;
        }
      } else {
        if (new_capacity <= SMALL_SIZE) {
          big_data_to_small_data();
        } else {
          if ((big_data_->count > 1 && capacity() == new_capacity) || (big_data_->capacity < new_capacity)) {
            create_new_big_data(new_capacity);
          }
        }
      }
    }
  }

  void shrink_to_fit() {
    if (!using_small_data) {
      if (size_ <= SMALL_SIZE) {
        big_data_to_small_data();
      } else if (size_ != capacity()) {
        create_new_big_data(size_);
      }
    }
  }

private:
  explicit socow_vector(std::size_t capacity)
      : size_(0)
      , using_small_data(false) {
    big_data_ = create_dynamic_data(capacity);
  }

  void full_clear() {
    if (!using_small_data) {
      dec_dynamic_data(big_data_, size_);
      using_small_data = true;
      size_ = 0;
    }
    clear();
  }

  void push_back_data(iterator left, iterator right) {
    while (left < right) {
      push_back(*left);
      left++;
    }
  }

  template <typename Type>
  void push_back_impl(Type&& value) {
    if (size_ == capacity()) {
      socow_vector temp(capacity() * 2 + 1);
      if (!using_small_data && big_data_->count > 1) {
        temp.push_back_data(current_data(), current_data() + size());
        new (temp.data() + size()) value_type(std::forward<Type>(value));
      } else {
        new (temp.data() + size()) value_type(std::forward<Type>(value));
        for (iterator it = current_data(); it < current_data() + size(); it++) {
          temp.push_back(std::move(*it));
        }
      }
      swap(temp);
    } else {
      check_shared();
      new (current_data() + size()) value_type(std::forward<Type>(value));
    }
    size_++;
  }

  template <typename Type>
  iterator insert_impl(const_iterator pos, Type&& value) {
    std::size_t diff = pos - current_data();
    push_back(std::forward<Type>(value));
    iterator it = end() - 1;
    while (it - begin() > diff) {
      std::iter_swap(it, it - 1);
      it--;
    }
    return it;
  }

  void swap_big_and_small_data(socow_vector& small_data, socow_vector& big_data) noexcept {
    dynamic_data* temp = big_data.big_data_;
    big_data.big_data_ = nullptr;
    std::uninitialized_move_n(small_data.small_data_.data(), small_data.size_, big_data.small_data_.data());
    std::destroy_n(small_data.small_data_.data(), small_data.size_);
    small_data.big_data_ = temp;
  }

  pointer current_data() {
    return (using_small_data ? small_data_.data() : big_data_->data());
  }

  void swap_small_data(value_type* data1, std::size_t size1, value_type* data2, std::size_t size2) {
    if (size1 > size2) {
      swap_small_data(data2, size2, data1, size1);
    } else {
      for (std::size_t i = 0; i < size1; i++) {
        std::swap(data1[i], data2[i]);
      }
      std::uninitialized_move_n(data2 + size1, size2 - size1, data1 + size1);
      std::destroy_n(data2 + size1, size2 - size1);
    }
  }

  void create_new_big_data(std::size_t size) {
    auto* new_big_data = create_dynamic_data(size);
    try {
      copy_data_to_new_place(big_data_, new_big_data->data());
    } catch (...) {
      operator delete(new_big_data);
      throw;
    }
    big_data_ = new_big_data;
  }

  void big_data_to_small_data() {
    auto* temp = big_data_;
    big_data_ = nullptr;
    try {
      copy_data_to_new_place(temp, small_data_.data());
    } catch (...) {
      big_data_ = temp;
      throw;
    }
    using_small_data = true;
  }

  void check_shared() {
    if (!using_small_data && big_data_->is_shared()) {
      socow_vector temp(capacity());
      temp.push_back_data(current_data(), current_data() + size());
      swap(temp);
    }
  }

  class dynamic_data {
    dynamic_data(std::size_t count, std::size_t capacity)
        : count(count)
        , capacity(capacity) {}

    value_type* data() {
      return reinterpret_cast<value_type*>(this + 1);
    }

    bool is_shared() {
      return count > 1;
    }

    std::size_t count;
    std::size_t capacity;
    std::array<T, 0> data_;

    friend class socow_vector;
  };

  template <typename E>
  void copy_data_to_new_place(dynamic_data* from, E* new_place) {
    if (from->is_shared()) {
      std::uninitialized_copy_n(from->data(), size_, new_place);
    } else {
      std::uninitialized_move_n(from->data(), size_, new_place);
    }
    dec_dynamic_data(from, size_);
  }

  dynamic_data* create_dynamic_data(std::size_t size) {
    auto* new_dynamic_data = static_cast<dynamic_data*>(operator new(sizeof(dynamic_data) + sizeof(value_type) * size));
    new (new_dynamic_data) dynamic_data{1, size};
    return new_dynamic_data;
  }

  void dec_dynamic_data(dynamic_data* data, std::size_t size) {
    data->count--;
    if (!data->count) {
      std::destroy_n(data->data(), size);
      operator delete(data);
    }
  }

  std::size_t size_;
  bool using_small_data;

  union {
    std::array<T, SMALL_SIZE> small_data_;
    dynamic_data* big_data_;
  };
};
