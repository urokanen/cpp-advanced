#pragma once

#include <cstddef>
#include <memory>
#include <utility>

namespace impl {
struct control_block {
public:
  void inc_ref() noexcept {
    strong_ref_count += 1;
    inc_weak_ref();
  }

  void inc_weak_ref() noexcept {
    weak_ref_count += 1;
  }

  void dec_ref() noexcept {
    strong_ref_count -= 1;
    if (strong_ref_count == 0) {
      clear_data();
    }
    dec_weak_ref();
  }

  void dec_weak_ref() noexcept {
    weak_ref_count -= 1;
    if (weak_ref_count == 0) {
      delete this;
    }
  }

  std::size_t ref_count() const noexcept {
    return strong_ref_count;
  }

protected:
  virtual ~control_block() noexcept = default;

  virtual void clear_data() noexcept = 0;

private:
  std::size_t strong_ref_count{0};
  std::size_t weak_ref_count{0};
};

template <typename T, typename Deleter>
struct control_block_ptr : control_block {
public:
  control_block_ptr(T* arg, Deleter&& deleter)
      : ptr(arg)
      , d(std::move(deleter)) {}

  ~control_block_ptr() override = default;

  void clear_data() noexcept override {
    d(ptr);
  }

private:
  T* ptr;
  [[no_unique_address]] Deleter d;
};

template <typename T>
struct control_block_obj : control_block {
public:
  template <typename... Args>
  control_block_obj(Args&&... args)
      : obj(std::forward<Args>(args)...) {}

  ~control_block_obj() override {}

  void clear_data() noexcept override {
    obj.~T();
  }

  T* get_ptr() noexcept {
    return &obj;
  }

private:
  union {
    T obj;
  };
};
} // namespace impl

template <typename T>
class shared_ptr {
public:
  shared_ptr() noexcept
      : cb(nullptr)
      , ptr(nullptr) {}

  shared_ptr(std::nullptr_t) noexcept
      : shared_ptr() {}

  template <typename Y>
  explicit shared_ptr(Y* arg)
    requires (std::is_convertible_v<Y*, T*>)
      : shared_ptr(arg, std::default_delete<Y>()) {}

  template <typename Y, typename Deleter>
  shared_ptr(Y* arg, Deleter deleter)
    requires (std::is_convertible_v<Y*, T*>)
      : ptr(arg) {
    try {
      cb = new impl::control_block_ptr(arg, std::move(deleter));
      cb->inc_ref();
    } catch (...) {
      deleter(arg);
      throw;
    }
  }

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other, T* arg) noexcept
      : shared_ptr(other.cb, arg) {}

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other, T* arg) noexcept
      : shared_ptr(other.cb, arg) {
    other.reset();
  }

  shared_ptr(const shared_ptr& other) noexcept
      : shared_ptr(other, other.get()) {}

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
      : shared_ptr(other, other.get()) {}

  shared_ptr(shared_ptr&& other) noexcept
      : shared_ptr(std::move(other), other.get()) {}

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
      : shared_ptr(std::move(other), other.get()) {}

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    shared_ptr copy(other);
    swap(copy);
    return *this;
  }

  template <typename Y>
  shared_ptr& operator=(const shared_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
  {
    shared_ptr copy(other);
    swap(copy);
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    shared_ptr copy(std::move(other));
    swap(copy);
    return *this;
  }

  template <typename Y>
  shared_ptr& operator=(shared_ptr<Y>&& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
  {
    shared_ptr copy(std::move(other));
    swap(copy);
    return *this;
  }

  void swap(shared_ptr& other) noexcept {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }

  T* get() const noexcept {
    return ptr;
  }

  explicit operator bool() const noexcept {
    return get() != nullptr;
  }

  T& operator*() const noexcept {
    return *get();
  }

  T* operator->() const noexcept {
    return get();
  }

  std::size_t use_count() const noexcept {
    return cb ? cb->ref_count() : 0;
  }

  void reset() noexcept {
    dec();
    cb = nullptr;
    ptr = nullptr;
  }

  template <typename Y>
  void reset(Y* new_ptr) {
    reset(new_ptr, std::default_delete<Y>());
  }

  template <typename Y, typename Deleter>
  void reset(Y* new_ptr, Deleter deleter) {
    shared_ptr temp(new_ptr, std::move(deleter));
    swap(temp);
  }

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return lhs.get() == rhs.get();
  }

  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return !(lhs == rhs);
  }

  ~shared_ptr() noexcept {
    dec();
  }

  template <typename Y>
  friend class shared_ptr;
  template <typename Y>
  friend class weak_ptr;
  template <typename Y, typename... Args>
  friend shared_ptr<Y> make_shared(Args&&... args);

private:
  void inc() noexcept {
    if (cb) {
      cb->inc_ref();
    }
  }

  void dec() noexcept {
    if (cb) {
      cb->dec_ref();
    }
  }

  template <typename Y>
  shared_ptr(impl::control_block* cb_, Y* ptr_) noexcept
      : cb(cb_)
      , ptr(ptr_) {
    inc();
  }

  impl::control_block* cb;
  T* ptr;
};

template <typename T>
class weak_ptr {
public:
  weak_ptr() noexcept
      : cb(nullptr)
      , ptr(nullptr) {}

  template <typename Y>
  weak_ptr(const shared_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
      : weak_ptr(other.cb, other.ptr) {}

  weak_ptr(const weak_ptr& other) noexcept
      : weak_ptr(other.cb, other.ptr) {}

  template <typename Y>
  weak_ptr(const weak_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
      : weak_ptr(other.cb, other.ptr) {}

  weak_ptr(weak_ptr&& other) noexcept
      : cb(std::move(other.cb))
      , ptr(std::move(other.ptr)) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  template <typename Y>
  weak_ptr(weak_ptr<Y>&& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
      : cb(std::move(other.cb))
      , ptr(std::move(other.ptr)) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  template <typename Y>
  void swap(weak_ptr<Y>& other) noexcept {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }

  template <typename Y>
  weak_ptr& operator=(const shared_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
  {
    weak_ptr copy(other);
    swap(copy);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    weak_ptr copy(other);
    swap(copy);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(const weak_ptr<Y>& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
  {
    weak_ptr copy(other);
    swap(copy);
    return *this;
  }

  weak_ptr& operator=(weak_ptr&& other) noexcept {
    weak_ptr copy(std::move(other));
    swap(copy);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(weak_ptr<Y>&& other) noexcept
    requires (std::is_convertible_v<Y*, T*>)
  {
    weak_ptr copy(std::move(other));
    swap(copy);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    if (cb && cb->ref_count() != 0) {
      return shared_ptr<T>(cb, ptr);
    }
    return shared_ptr<T>();
  }

  void reset() noexcept {
    dec();
    ptr = nullptr;
    cb = nullptr;
  }

  ~weak_ptr() {
    dec();
  }

  template <typename Y>
  friend class weak_ptr;

private:
  weak_ptr(impl::control_block* cb_, T* ptr_) noexcept
      : cb(cb_)
      , ptr(ptr_) {
    inc();
  }

  void dec() noexcept {
    if (cb) {
      cb->dec_weak_ref();
    }
  }

  void inc() noexcept {
    if (cb) {
      cb->inc_weak_ref();
    }
  }

  impl::control_block* cb;
  T* ptr;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* new_cb = new impl::control_block_obj<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(new_cb, new_cb->get_ptr());
}
