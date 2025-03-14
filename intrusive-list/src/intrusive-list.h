#pragma once

#include <cstddef>
#include <iostream>
#include <iterator>
#include <type_traits>

namespace intrusive {

class default_tag;

class list_element_base {
public:
  list_element_base() noexcept = default;

  list_element_base(list_element_base&& other) noexcept;

  list_element_base(const list_element_base&) noexcept {}

  list_element_base& operator=(const list_element_base& other) noexcept;

  list_element_base& operator=(list_element_base&& other) noexcept;

  ~list_element_base() noexcept;

  void unlink() noexcept;

  void link(list_element_base* pos) noexcept;

  bool is_linked() const noexcept;

  void link_on_this() noexcept;

  list_element_base* next{this};
  list_element_base* prev{this};
};

template <typename Tag = default_tag>
class list_element : private list_element_base {
  template <typename T, typename Tag2>
  friend class list;
  template <typename T, typename Tag2>
  friend class list_iterator;
};

template <typename T, typename Tag = default_tag>
class list_iterator {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using node = list_element<Tag>;
  using value_type = T;
  using reference = T&;
  using pointer = T*;

  list_iterator() noexcept = default;

  operator list_iterator<const T, Tag>() const noexcept {
    return {iter_node_};
  }

  pointer operator->() const noexcept {
    return static_cast<T*>(iter_node_);
  }

  reference operator*() const noexcept {
    return *operator->();
  }

  list_iterator& operator++() noexcept {
    iter_node_ = static_cast<node*>(iter_node_->next);
    return *this;
  }

  list_iterator operator++(int) noexcept {
    list_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  list_iterator& operator--() noexcept {
    iter_node_ = static_cast<node*>(iter_node_->prev);
    return *this;
  }

  list_iterator operator--(int) noexcept {
    list_iterator tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const list_iterator& lhs, const list_iterator& rhs) noexcept {
    return lhs.iter_node_ == rhs.iter_node_;
  }

  friend bool operator!=(const list_iterator& lhs, const list_iterator& rhs) noexcept {
    return !(lhs.iter_node_ == rhs.iter_node_);
  }

private:
  template <typename E, typename Tag2>
  friend class list_iterator;
  template <typename E, typename Tag2>
  friend class list;

  list_iterator(node* iter_node) noexcept
      : iter_node_(iter_node) {}

  node* iter_node_;
};

template <typename T, typename Tag = default_tag>
class list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>, "T must derive from list_element");

public:
  using node = list_element<Tag>;
  using iterator = list_iterator<T, Tag>;
  using const_iterator = list_iterator<const T, Tag>;

public:
  list() noexcept = default;

  ~list() = default;

  list(const list&) = delete;
  list& operator=(const list&) = delete;

  list(list&& other) noexcept = default;

  list& operator=(list&& other) noexcept = default;

  bool empty() const noexcept {
    return !sentinel.is_linked();
  }

  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  T& front() noexcept {
    return *(begin());
  }

  const T& front() const noexcept {
    return *(begin());
  }

  T& back() noexcept {
    return *(std::prev(end()));
  }

  const T& back() const noexcept {
    return *(std::prev(end()));
  }

  void push_front(T& value) noexcept {
    insert(begin(), value);
  }

  void push_back(T& value) noexcept {
    insert(end(), value);
  }

  void pop_front() noexcept {
    erase(begin());
  }

  void pop_back() noexcept {
    erase(--end());
  }

  void clear() noexcept {
    sentinel.unlink();
  }

  iterator begin() noexcept {
    return {static_cast<node*>(sentinel.next)};
  }

  const_iterator begin() const noexcept {
    return {static_cast<node*>(sentinel.next)};
  }

  iterator end() noexcept {
    return {&sentinel};
  }

  const_iterator end() const noexcept {
    return {const_cast<node*>(&sentinel)};
  }

  iterator insert(const_iterator pos, T& value) noexcept {
    auto previous = pos.iter_node_->prev;
    if (pos.iter_node_ != &value) {
      static_cast<node&>(value).unlink();
      static_cast<node&>(value).link(pos.iter_node_);
    }
    return {static_cast<node*>(previous->next)};
  }

  iterator erase(const_iterator pos) noexcept {
    auto previous = pos.iter_node_->next;
    pos.iter_node_->unlink();
    return {static_cast<node*>(previous)};
  }

  void splice(const_iterator pos, [[maybe_unused]] list& other, const_iterator first, const_iterator last) noexcept {
    if (first != last) {
      auto prv1 = first.iter_node_->prev;
      auto prv2 = last.iter_node_->prev;
      auto prv3 = pos.iter_node_->prev;
      connect(prv3, first.iter_node_);
      connect(prv1, last.iter_node_);
      connect(prv2, pos.iter_node_);
    }
  }

private:
  void connect(list_element_base* prv, list_element_base* nxt) {
    prv->next = nxt;
    nxt->prev = prv;
  }

  node sentinel;
};

extern template class list<list_element<default_tag>>;
} // namespace intrusive
