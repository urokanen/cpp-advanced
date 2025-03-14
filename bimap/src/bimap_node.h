#pragma once

#include "sentinel_node.h"

namespace bimap_impl {
template <typename Left, typename Right>
class bimap_node : private sentinel_node {
public:
  template <typename Tag>
  using value_t = std::conditional_t<std::is_same_v<Tag, left_tag>, Left, Right>;

  template <typename L, typename R>
  bimap_node(L&& left, R&& right)
      : left(std::forward<L>(left))
      , right(std::forward<R>(right)) {}

  bimap_node(bimap_node&& other)
      : sentinel_node(std::move(other))
      , left(std::move(other.left))
      , right(std::move(other.right)) {}

  template <typename Tag>
  const value_t<Tag>& get_value() const noexcept {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return left;
    } else {
      return right;
    }
  }

private:
  template <typename Tag, typename Current, typename Another, typename Compare>
  friend class map;
  template <typename L, typename R, typename CompareLeft, typename CompareRight>
  friend class ::bimap;
  template <typename Tag, typename Current, typename Another>
  friend class bimap_iterator;

  Left left;
  Right right;
};
} // namespace bimap_impl
