#pragma once

#include "node.h"

#include <utility>

namespace bimap_impl {
class sentinel_node
    : private node<left_tag>
    , private node<right_tag> {
private:
  template <typename Tag, typename Current, typename Another, typename Compare>
  friend class map;
  template <typename L, typename R, typename CompareLeft, typename CompareRight>
  friend class ::bimap;
  template <typename Tag, typename Current, typename Another>
  friend class bimap_iterator;
};
} // namespace bimap_impl
