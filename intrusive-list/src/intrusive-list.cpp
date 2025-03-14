#include "intrusive-list.h"

namespace intrusive {
list_element_base::list_element_base(list_element_base&& other) noexcept
    : list_element_base() {
  *this = std::move(other);
}

list_element_base& list_element_base::operator=([[maybe_unused]] const list_element_base& other) noexcept {
  if (this != &other) {
    unlink();
  }
  return *this;
}

list_element_base& list_element_base::operator=(list_element_base&& other) noexcept {
  if (this != &other) {
    unlink();
    if (other.is_linked()) {
      next = other.next;
      prev = other.prev;
      other.next->prev = this;
      other.prev->next = this;
      other.link_on_this();
    } else {
      link_on_this();
    }
  }
  return *this;
}

list_element_base::~list_element_base() noexcept {
  unlink();
}

void list_element_base::link_on_this() noexcept {
  next = prev = this;
}

bool list_element_base::is_linked() const noexcept {
  return next != this;
}

void list_element_base::unlink() noexcept {
  next->prev = prev;
  prev->next = next;
  link_on_this();
}

void list_element_base::link(list_element_base* pos) noexcept {
  prev = pos->prev;
  prev->next = this;
  pos->prev = this;
  next = pos;
}

template class list<list_element<default_tag>>;
} // namespace intrusive
