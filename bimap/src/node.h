#pragma once

#include <memory>
#include <random>

template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
class bimap;

namespace bimap_impl {
template <typename Tag>
class node {
public:
  node() noexcept
      : value(random_int()) {}

  node(node&& other) noexcept
      : value(other.value)
      , left(other.left)
      , right(other.right)
      , pred(other.pred) {
    if (left) {
      left->pred = this;
    }
    if (right) {
      right->pred = this;
    }
    clear_node(&other);
  }

  const node* get_right(node* temp) const noexcept {
    return !temp ? nullptr : (temp->right ? get_right(temp->right) : temp);
  }

  const node* get_left(node* temp) const noexcept {
    return !temp ? nullptr : (temp->left ? get_left(temp->left) : temp);
  }

  const node* get_next() const noexcept {
    auto res = get_left(right);
    return res ? res : up_right(this);
  }

  const node* get_prev() const noexcept {
    auto res = get_right(left);
    return res ? res : up_left(this);
  }

  const node* up_right(const node* temp) const noexcept {
    while (temp->pred && temp->pred->right == temp) {
      temp = temp->pred;
    }
    return temp->pred ? temp->pred : temp;
  }

  const node* up_left(const node* temp) const noexcept {
    while (temp->pred && temp->pred->left == temp) {
      temp = temp->pred;
    }
    return temp->pred ? temp->pred : temp;
  }

  void set_new_node(node* new_node) noexcept {
    if (pred == nullptr || value > new_node->value) {
      set_new_left(new_node);
    } else {
      set_new_right(new_node);
    }
  }

  void set_new_left(node* new_node) noexcept {
    if (left) {
      new_node->left = left;
      left->pred = new_node;
    }
    new_node->pred = this;
    left = new_node;
  }

  static void set_left(node* pred_, node* left_) noexcept {
    pred_->left = left_;
    if (pred_->left) {
      pred_->left->pred = pred_;
    }
  }

  static void set_right(node* pred_, node* right_) noexcept {
    pred_->right = right_;
    if (pred_->right) {
      pred_->right->pred = pred_;
    }
  }

  void clear_node(node* node) noexcept {
    node->left = nullptr;
    node->pred = nullptr;
    node->right = nullptr;
  }

  void set_pred(node* new_node) noexcept {
    if (pred->left == this) {
      set_left(pred, new_node);
    } else {
      set_right(pred, new_node);
    }
  }

  void set_new_right(node* new_node) noexcept {
    if (left) {
      set_left(new_node, left);
      left = nullptr;
    }
    set_pred(new_node);
    new_node->right = this;
    pred = new_node;
  }

  void erase() noexcept {
    auto* node = merge(left, right);
    set_pred(node);
    clear_node(this);
  }

  node* merge(node* left, node* right) noexcept {
    if (!left) {
      return right;
    }
    if (!right) {
      return left;
    }
    if (left->value > right->value) {
      auto* res = merge(left->right, right);
      set_right(left, res);
      return left;
    } else {
      auto* res = merge(left, right->left);
      set_left(right, res);
      return right;
    }
  }

  friend void swap(node& lhs, node& rhs) noexcept {
    auto lhs_left = lhs.left;
    set_left(&lhs, rhs.left);
    set_left(&rhs, lhs_left);
  }

  int random_int() noexcept {
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, INT_MAX);
    return distribution(generator);
  }

  friend bool operator==(const node& lhs, const node& rhs) noexcept {
    return lhs.pred == rhs.pred && lhs.left == rhs.left && lhs.right == rhs.right;
  }

  int value;
  node* left = nullptr;
  node* right = nullptr;
  node* pred = nullptr;
};

class left_tag;
class right_tag;
} // namespace bimap_impl
