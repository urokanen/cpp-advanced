#pragma once

#include <cstddef>
#include <cstring>
#include <exception>
#include <memory>

class bad_function_call : public std::exception {
public:
  const char* what() const noexcept override {
    return "bad function call";
  }
};

template <typename F>
class function;

namespace function_impl {
static constexpr std::size_t SIZE = sizeof(void*);
using storage_t = std::byte[SIZE];
template <typename T>
concept SmallType =
    (sizeof(T) <= SIZE) && (alignof(T) <= alignof(std::max_align_t)) && std::is_nothrow_move_constructible_v<T>;

template <typename T>
class interface;

template <typename R, typename... Args>
class interface<R(Args...)> {
public:
  virtual R operator()(storage_t& src, Args&&... args) const = 0;
  virtual void copy(const storage_t& src, storage_t& dst) const = 0;
  virtual void move(storage_t&& src, storage_t& dst) const noexcept = 0;
  virtual void destroy(storage_t& data) const noexcept = 0;
};

template <typename F, typename T>
class model;

template <typename T, typename R, typename... Args>
class model<T, R(Args...)> final : public interface<R(Args...)> {
public:
  R operator()(storage_t& src, Args&&... args) const override {
    return (*get_data(src))(std::forward<Args>(args)...);
  }

  void copy(const storage_t& src, storage_t& dst) const override {
    T* new_obj = new T(*get_data(src));
    new (&dst) T*(new_obj);
  }

  void move(storage_t&& src, storage_t& dst) const noexcept override {
    new (&dst) T*(get_data(src));
    *std::launder(reinterpret_cast<T**>(&src)) = nullptr;
  }

  void destroy(storage_t& data) const noexcept override {
    delete get_data(data);
  }

  void get_func(T&& func, storage_t& storage) {
    T* ptr = new T(std::move(func));
    new (storage) T*(ptr);
  }

  static T* get_data(storage_t& src) noexcept {
    return *std::launder(reinterpret_cast<T**>(&src));
  }

  static const T* get_data(const storage_t& src) noexcept {
    return *std::launder(reinterpret_cast<const T* const*>(&src));
  }
};

template <typename R, typename... Args>
class model<void, R(Args...)> final : public interface<R(Args...)> {
public:
  R operator()([[maybe_unused]] storage_t& src, [[maybe_unused]] Args&&... args) const override {
    throw bad_function_call{};
  }

  void copy([[maybe_unused]] const storage_t& src, [[maybe_unused]] storage_t& dst) const override {}

  void move([[maybe_unused]] storage_t&& src, [[maybe_unused]] storage_t& dst) const noexcept override {}

  void destroy([[maybe_unused]] storage_t& data) const noexcept override {}
};

template <typename T, typename R, typename... Args>
  requires SmallType<T>
class model<T, R(Args...)> final : public interface<R(Args...)> {
public:
  R operator()(storage_t& src, Args&&... args) const override {
    return (*get_data(src))(std::forward<Args>(args)...);
  }

  void copy(const storage_t& src, storage_t& dst) const override {
    new (&dst) T(*get_data(src));
  }

  void move(storage_t&& src, storage_t& dst) const noexcept override {
    new (&dst) T(std::move(*get_data(src)));
    destroy(src);
  }

  void destroy(storage_t& data) const noexcept override {
    get_data(data)->~T();
  }

  void get_func(T&& func, storage_t& storage) {
    new (storage) T(std::move(func));
  }

  static T* get_data(storage_t& src) noexcept {
    return std::launder(reinterpret_cast<T*>(&src));
  }

  static const T* get_data(const storage_t& src) noexcept {
    return std::launder(reinterpret_cast<const T*>(&src));
  }
};

template <typename F, typename R, typename... Args>
inline static model<F, R(Args...)> models;
} // namespace function_impl

template <typename R, typename... Args>
class function<R(Args...)> {
public:
  function() noexcept
      : control(&function_impl::models<void, R, Args...>) {}

  template <typename F>
  function(F func)
      : control(&function_impl::models<F, R, Args...>) {
    (&function_impl::models<F, R, Args...>)->get_func(std::move(func), storage);
  }

  function(const function& other)
      : control(other.control) {
    control->copy(other.storage, storage);
  }

  function(function&& other) noexcept
      : control(other.control) {
    control->move(std::move(other.storage), storage);
    other.control = &function_impl::models<void, R, Args...>;
  }

  function& operator=(const function& other) {
    if (this != &other) {
      *this = function(other);
    }
    return *this;
  }

  function& operator=(function&& other) noexcept {
    if (this != &other) {
      control->destroy(storage);
      control = other.control;
      control->move(std::move(other.storage), storage);
      other.control = &function_impl::models<void, R, Args...>;
    }
    return *this;
  }

  ~function() {
    control->destroy(storage);
  }

  explicit operator bool() const noexcept {
    return control != &function_impl::models<void, R, Args...>;
  }

  R operator()(Args... args) const {
    return control->operator()(storage, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    if (control == &function_impl::models<T, R, Args...>) {
      return (&function_impl::models<T, R, Args...>)->get_data(storage);
    }
    return nullptr;
  }

  template <typename T>
  const T* target() const noexcept {
    return const_cast<function*>(this)->target<T>();
  }

private:
  alignas(std::max_align_t) mutable function_impl::storage_t storage;
  function_impl::interface<R(Args...)>* control;
};
