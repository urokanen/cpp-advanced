#pragma once

#include "map.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>

template <
    typename Left,
    typename Right,
    typename CompareLeft = std::less<Left>,
    typename CompareRight = std::less<Right>>
class bimap
    : private bimap_impl::map<bimap_impl::left_tag, Left, Right, CompareLeft>
    , private bimap_impl::map<bimap_impl::right_tag, Right, Left, CompareRight> {
public:
  using left_t = Left;
  using right_t = Right;
  using left_node = bimap_impl::node<bimap_impl::left_tag>;
  using right_node = bimap_impl::node<bimap_impl::right_tag>;
  using left_iterator = bimap_impl::bimap_iterator<bimap_impl::left_tag, Left, Right>;
  using right_iterator = bimap_impl::bimap_iterator<bimap_impl::right_tag, Right, Left>;
  using node_t = bimap_impl::bimap_node<left_t, right_t>;
  using left_map = bimap_impl::map<bimap_impl::left_tag, Left, Right, CompareLeft>;
  using right_map = bimap_impl::map<bimap_impl::right_tag, Right, Left, CompareRight>;

public:
  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight())
      : left_map(std::move(compare_left))
      , right_map(std::move(compare_right)) {}

  bimap(const bimap& other)
      : left_map(static_cast<const left_map&>(other).compare)
      , right_map(static_cast<const right_map&>(other).compare) {
    try {
      for (left_iterator it = other.begin_left(); it != other.end_left(); it++) {
        insert(*it, *it.flip());
      }
    } catch (...) {
      erase_left(begin_left(), end_left());
      throw;
    }
  }

  bimap(bimap&& other) noexcept
      : left_map(std::move(static_cast<left_map&>(other).compare))
      , right_map(std::move(static_cast<right_map&>(other).compare))
      , size_(other.size_)
      , base(std::move(other.base)) {
    other.size_ = 0;
  }

  bimap& operator=(const bimap& other) {
    if (this != &other) {
      *this = bimap(other);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap tmp(std::move(other));
      swap(tmp, *this);
    }
    return *this;
  }

  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  friend void swap(bimap& lhs, bimap& rhs) noexcept {
    using std::swap;
    swap(lhs.size_, rhs.size_);
    swap(lhs.left(), rhs.left());
    swap(lhs.right(), rhs.right());
    swap(static_cast<left_map&>(lhs), static_cast<left_map&>(rhs));
    swap(static_cast<right_map&>(lhs), static_cast<right_map&>(rhs));
  }

  left_iterator insert(const left_t& left, const right_t& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(const left_t& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, const right_t& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) {
    left_iterator ans = this->left_map::erase(it);
    size_--;
    return ans;
  }

  right_iterator erase_right(right_iterator it) {
    right_iterator ans = this->right_map::erase(it);
    size_--;
    return ans;
  }

  bool erase_left(const left_t& value) {
    if (this->left_map::erase(value, &left())) {
      size_--;
      return true;
    }
    return false;
  }

  bool erase_right(const right_t& value) {
    if (this->right_map::erase(value, &right())) {
      size_--;
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    size_t count = std::distance(first, last);
    left_iterator ans = this->left_map::erase(first, last);
    size_ -= count;
    return ans;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    size_t count = std::distance(first, last);
    right_iterator ans = this->right_map::erase(first, last);
    size_ -= count;
    return ans;
  }

  left_iterator find_left(const left_t& value) const {
    return this->left_map::find(value, &left());
  }

  right_iterator find_right(const right_t& value) const {
    return this->right_map::find(value, &right());
  }

  const right_t& at_left(const left_t& key) const {
    return this->left_map::at(key, &left());
  }

  const left_t& at_right(const right_t& key) const {
    return this->right_map::at(key, &right());
  }

  const right_t& at_left_or_default(const left_t& key)
    requires (std::is_default_constructible_v<right_t>)
  {
    if (this->left_map::at_or_default(key, &left())) {
      return at_left(key);
    } else {
      right_t default_value = right_t();
      if (this->right_map::at_or_default(default_value, &right())) {
        auto* left_node_ = lower_bound_left(key).get_node();
        auto* right_node_ = (++lower_bound_right(default_value)).get_node();
        auto it = find_right(default_value);
        auto* new_node = new node_t(key, std::move(default_value));
        erase_right(it);
        return *insert_node(left_node_, right_node_, new_node).flip();
      }
      return *insert(key, std::move(default_value)).flip();
    }
  }

  const left_t& at_right_or_default(const right_t& key)
    requires (std::is_default_constructible_v<left_t>)
  {
    if (this->right_map::at_or_default(key, &right())) {
      return at_right(key);
    } else {
      left_t default_value = left_t();
      if (this->left_map::at_or_default(default_value, &left())) {
        auto* left_node_ = (++lower_bound_left(default_value)).get_node();
        auto* right_node_ = lower_bound_right(key).get_node();
        auto it = find_left(default_value);
        auto* new_node = new node_t(std::move(default_value), key);
        erase_left(it);
        return *insert_node(left_node_, right_node_, new_node);
      }
      return *insert(std::move(default_value), key);
    }
  }

  left_iterator lower_bound_left(const left_t& value) const {
    return this->left_map::lower_bound(value, &left());
  }

  left_iterator upper_bound_left(const left_t& value) const {
    return this->left_map::upper_bound(value, &left());
  }

  right_iterator lower_bound_right(const right_t& value) const {
    return this->right_map::lower_bound(value, &right());
  }

  right_iterator upper_bound_right(const right_t& value) const {
    return this->right_map::upper_bound(value, &right());
  }

  left_iterator begin_left() const noexcept {
    return this->left_map::begin(&left());
  }

  left_iterator end_left() const noexcept {
    return this->left_map::end(&left());
  }

  right_iterator begin_right() const noexcept {
    return this->right_map::begin(&right());
  }

  right_iterator end_right() const noexcept {
    return this->right_map::end(&right());
  }

  bool empty() const noexcept {
    return left().left == nullptr;
  }

  std::size_t size() const noexcept {
    return size_;
  }

  friend bool operator==(const bimap& lhs, const bimap& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    left_iterator lhs_it = lhs.begin_left();
    left_iterator rhs_it = rhs.begin_left();
    while (lhs_it != lhs.end_left() && rhs_it != rhs.end_left()) {
      if (static_cast<const left_map&>(lhs).compare(*lhs_it, *rhs_it) ||
          static_cast<const left_map&>(lhs).compare(*rhs_it, *lhs_it) ||
          static_cast<const right_map&>(lhs).compare(*lhs_it.flip(), *rhs_it.flip()) ||
          static_cast<const right_map&>(lhs).compare(*rhs_it.flip(), *lhs_it.flip())) {
        return false;
      }
      lhs_it++;
      rhs_it++;
    }
    return true;
  }

  friend bool operator!=(const bimap& lhs, const bimap& rhs) {
    return !(lhs == rhs);
  }

private:
  left_node& left() noexcept {
    return static_cast<left_node&>(base);
  }

  const left_node& left() const noexcept {
    return static_cast<const left_node&>(base);
  }

  right_node& right() noexcept {
    return static_cast<right_node&>(base);
  }

  const right_node& right() const noexcept {
    return static_cast<const right_node&>(base);
  }

  template <typename Left_t, typename Right_t>
  left_iterator insert_impl(Left_t&& left, Right_t&& right) {
    if (find_left(left) == end_left() && find_right(right) == end_right()) {
      auto* left_node_ = lower_bound_left(left).get_node();
      auto* right_node_ = lower_bound_right(right).get_node();
      auto* new_node = new node_t(std::forward<Left_t>(left), std::forward<Right_t>(right));
      return insert_node(left_node_, right_node_, new_node);
    }
    return end_left();
  }

  left_iterator insert_node(left_node* left_node_, right_node* right_node_, node_t* new_node) {
    left_node_->set_new_node(new_node);
    right_node_->set_new_node(new_node);
    size_++;
    return left_iterator(new_node);
  }

  size_t size_ = 0;
  bimap_impl::sentinel_node base;
};
