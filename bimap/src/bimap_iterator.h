#pragma once

#include "bimap_node.h"

#include <cstddef>
#include <iterator>

namespace bimap_impl {
template <typename Tag>
struct opposite_tag;

template <>
struct opposite_tag<left_tag> {
  using type = right_tag;
};

template <>
struct opposite_tag<right_tag> {
  using type = left_tag;
};

template <typename Tag, typename Current, typename Another>
class bimap_iterator {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Current;
  using reference = const Current&;
  using pointer = const Current*;

  bimap_iterator() noexcept = default;

  bimap_iterator(const node<Tag>* arg) noexcept
      : node_(arg) {}

  reference operator*() const noexcept {
    return *operator->();
  }

  pointer operator->() const noexcept {
    if constexpr (std::is_same<Tag, left_tag>::value) {
      return &(static_cast<const bimap_node<Current, Another>*>(node_)->left);
    } else if constexpr (std::is_same<Tag, right_tag>::value) {
      return &(static_cast<const bimap_node<Another, Current>*>(node_)->right);
    }
  }

  bimap_iterator<Tag, Current, Another>& operator++() noexcept {
    node_ = node_->get_next();
    return *this;
  }

  bimap_iterator<Tag, Current, Another> operator++(int) noexcept {
    bimap_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bimap_iterator<Tag, Current, Another>& operator--() noexcept {
    node_ = node_->get_prev();
    return *this;
  }

  bimap_iterator<Tag, Current, Another> operator--(int) noexcept {
    bimap_iterator tmp = *this;
    --(*this);
    return tmp;
  }

  bimap_iterator<typename opposite_tag<Tag>::type, Another, Current> flip() const noexcept {
    if constexpr (std::is_same<Tag, left_tag>::value) {
      auto obj = static_cast<const sentinel_node*>(node_);
      const node<typename opposite_tag<Tag>::type>* node_ptr(obj);
      return bimap_iterator<typename opposite_tag<Tag>::type, Another, Current>(node_ptr);
    } else if constexpr (std::is_same<Tag, right_tag>::value) {
      auto obj = static_cast<const sentinel_node*>(node_);
      const node<typename opposite_tag<Tag>::type>* node_ptr(obj);
      return bimap_iterator<typename opposite_tag<Tag>::type, Another, Current>(node_ptr);
    }
  }

  friend bool operator==(const bimap_iterator& lhs, const bimap_iterator& rhs) noexcept {
    return *lhs.node_ == *rhs.node_;
  }

  friend bool operator!=(const bimap_iterator& lhs, const bimap_iterator& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  template <typename Tag2, typename Current2, typename Another2, typename Compare>
  friend class map;
  template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
  friend class ::bimap;

  node<Tag>* get_node() {
    return const_cast<node<Tag>*>(node_);
  }

  const node<Tag>* node_;
};
} // namespace bimap_impl
