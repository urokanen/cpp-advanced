#pragma once

#include "bimap_iterator.h"

namespace bimap_impl {
template <typename Tag, typename Current, typename Another, typename Compare>
class map {
public:
  using iterator = bimap_iterator<Tag, Current, Another>;
  using value_t = Current;
  using flip_t = Another;

  using node_t =
      std::conditional_t<std::is_same_v<Tag, left_tag>, bimap_node<Current, Another>, bimap_node<Another, Current>>;

  map(const Compare& compare)
      : compare(compare) {}

  map(Compare&& compare)
      : compare(std::move(compare)) {}

  friend void swap(map& lhs, map& rhs) noexcept {
    using std::swap;
    swap(lhs.compare, rhs.compare);
  }

  iterator erase(iterator it) {
    auto res = it;
    res++;
    it.get_node()->erase();
    it.flip().get_node()->erase();
    delete (static_cast<node_t*>(it.get_node()));
    return res;
  }

  bool erase(const value_t& value, node<Tag>* node) {
    auto it = lower_bound(value, node);
    if (it != end(node) && *it == value) {
      erase(it);
      return true;
    }
    return false;
  }

  iterator erase(iterator first, iterator last) {
    while (first != last) {
      erase(first++);
    }
    return last;
  }

  iterator find(const value_t& value, const node<Tag>* node) const {
    auto res = lower_bound(value, node);
    return res == end(node) || compare(*res, value) || compare(value, *res) ? end(node) : res;
  }

  const flip_t& at(const value_t& key, const node<Tag>* node) const {
    auto it = lower_bound(key, node);
    if (it == end(node) || *it != key) {
      throw std::out_of_range("Index is out of range");
    } else {
      return *(it.flip());
    }
  }

  bool at_or_default(const value_t& key, node<Tag>* node) {
    auto it = lower_bound(key, node);
    return (it != end(node) && *it == key);
  }

  iterator lower_bound(const value_t& value, const node<Tag>* node) const {
    auto res = lower_bound_impl(value, node->left);
    return res == nullptr ? end(node) : iterator(res);
  }

  const node<Tag>* lower_bound_impl(const value_t& value, const node<Tag>* node) const {
    if (!node) {
      return nullptr;
    }
    if (compare(static_cast<const node_t*>(node)->template get_value<Tag>(), value)) {
      return lower_bound_impl(value, node->right);
    } else {
      auto res = lower_bound_impl(value, node->left);
      if (res) {
        return !compare(static_cast<const node_t*>(res)->template get_value<Tag>(), value) ? res : node;
      } else {
        return node;
      }
    }
  }

  iterator upper_bound(const value_t& value, const node<Tag>* node) const {
    auto it = lower_bound(value, node);
    if (it != end(node) && !compare(value, *it) && !compare(*it, value)) {
      it++;
    }
    return it;
  }

  iterator begin(const node<Tag>* node) const noexcept {
    if (!node->left) {
      return end(node);
    }
    return iterator(node->get_left(node->left));
  }

  iterator end(const node<Tag>* node) const noexcept {
    return iterator(node);
  }

private:
  template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
  friend class ::bimap;

  [[no_unique_address]] Compare compare;
};
} // namespace bimap_impl
