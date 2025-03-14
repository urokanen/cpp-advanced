#pragma once

#include <intrusive-list.h>

#include <functional>

namespace signals {
class signal_tag;

template <typename T>
class signal;

template <typename... Args>
class signal<void(Args...)> {
public:
  class connection;

  using slot = std::function<void(Args...)>;
  using iter = intrusive::list_iterator<connection, signal_tag>;
  using elem = intrusive::list_element<signal_tag>;

  class iterator_token {
  public:
    explicit iterator_token(signal* sig) noexcept
        : _sig(sig)
        , it(sig->_list.begin())
        , prev(_sig->_token) {
      _sig->_token = this;
      for (auto i = _sig->_list.begin(); i != _sig->_list.end(); i++) {
        i->_it = i->_strong_it = this;
      }
    }

    ~iterator_token() noexcept {
      if (_sig) {
        _sig->_token = prev;
        for (auto i = _sig->_list.begin(); i != _sig->_list.end(); i++) {
          i->_it = i->_strong_it = prev;
        }
      }
    }

    signal* _sig;
    iter it;
    iterator_token* prev = nullptr;
  };

  class connection : public elem {
  public:
    connection() noexcept
        : _slot(slot()) {}

    explicit connection(signal* sig, slot slot) noexcept
        : _slot(std::move(slot)) {
      sig->_list.push_back(*this);
    }

    connection(connection&& other) noexcept
        : elem(std::move(other))
        , _it(other._it)
        , _strong_it(other._strong_it)
        , _slot(std::move(other._slot)) {
      set_right_iterator(other);
    }

    friend void swap(connection& lhs, connection& rhs) noexcept {
      using std::swap;
      swap(lhs._it, rhs._it);
      swap(lhs._slot, rhs._slot);
      swap(lhs._strong_it, rhs._strong_it);
      swap(static_cast<elem&>(lhs), static_cast<elem&>(rhs));
    }

    connection& operator=(connection&& other) noexcept {
      if (this != &other) {
        disconnect();
        connection temp(std::move(other));
        set_right_iterator(temp);
        swap(temp, *this);
      }
      return *this;
    }

    ~connection() noexcept {
      disconnect();
    }

    void disconnect() noexcept {
      _slot = nullptr;
      for (auto* it = _it; it != nullptr; it = it->prev) {
        if (std::to_address(it->it) == this) {
          it->it++;
        }
      }
      _it = nullptr;
      if (need_unlink) {
        unlink();
      }
    }

  private:
    void set_right_iterator(connection& other) {
      if (_strong_it && std::to_address(_strong_it->it) == &other) {
        _strong_it->it = iter(this);
      }
    }

    template <typename T>
    friend class signal;

    iterator_token* _it = nullptr;
    iterator_token* _strong_it = nullptr;
    bool need_unlink = true;
    slot _slot;
  };

  using list = intrusive::list<connection, signal_tag>;

  signal() noexcept {}

  signal(const signal&) = delete;
  signal& operator=(const signal&) = delete;

  signal(signal&& other) noexcept
      : _list(std::move(other._list))
      , _token(other._token) {
    for (auto* it = _token; it != nullptr; it = it->prev) {
      it->_sig = this;
    }
    other._token = nullptr;
  }

  friend void swap(signal& lhs, signal& rhs) noexcept {
    using std::swap;
    swap(lhs._list, rhs._list);
    swap(lhs._token, rhs._token);
    fill(lhs, &lhs);
    fill(rhs, &rhs);
  }

  signal& operator=(signal&& other) noexcept {
    if (this != &other) {
      signal temp(std::move(other));
      swap(temp, *this);
    }
    return *this;
  }

  ~signal() {
    fill(*this, nullptr);
    for (auto it = _list.begin(); it != _list.end(); it++) {
      it->need_unlink = false;
    }
    clear(*this);
  }

  [[nodiscard]] connection connect(slot slot) noexcept {
    return connection(this, std::move(slot));
  }

  void operator()(Args... args) const {
    iterator_token token(const_cast<signal*>(this));
    while (token.it != token._sig->_list.end()) {
      auto current = token.it++;
      current->_slot(args...);
      if (!token._sig) {
        return;
      }
    }
  }

private:
  static void clear(signal& sig) {
    for (auto it = sig._list.begin(); it != sig._list.end(); it++) {
      it->disconnect();
    }
  }

  static void fill(signal& sig, signal* value) {
    for (auto* it = sig._token; it != nullptr; it = it->prev) {
      it->_sig = value;
    }
  }

  list _list;
  iterator_token* _token = nullptr;
};

} // namespace signals
