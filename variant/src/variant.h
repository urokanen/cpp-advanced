#pragma once

#include "variadic_union.h"

#include <compare>

template <size_t I, typename... Types>
constexpr variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v) {
  if (v.index_ != I) {
    throw bad_variant_access{};
  }
  return v.data.template get<I>();
}

template <size_t I, typename... Types>
constexpr variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v) {
  if (v.index_ != I) {
    throw bad_variant_access{};
  }
  return std::move(v.data.template get<I>());
}

template <size_t I, typename... Types>
constexpr const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v) {
  if (v.index_ != I) {
    throw bad_variant_access{};
  }
  return v.data.template get<I>();
}

template <size_t I, typename... Types>
constexpr const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v) {
  if (v.index_ != I) {
    throw bad_variant_access{};
  }
  return std::move(v.data.template get<I>());
}

template <typename T, typename... Types>
constexpr T& get(variant<Types...>& v) {
  return get<impl::get_index_by_type_v<T, Types...>>(v);
}

template <typename T, typename... Types>
constexpr T&& get(variant<Types...>&& v) {
  return std::move(get<impl::get_index_by_type_v<T, Types...>>(v));
}

template <typename T, typename... Types>
constexpr const T& get(const variant<Types...>& v) {
  return get<impl::get_index_by_type_v<T, Types...>>(v);
}

template <typename T, typename... Types>
constexpr const T&& get(const variant<Types...>&& v) {
  return std::move(get<impl::get_index_by_type_v<T, Types...>>(v));
}

namespace impl {
template <typename R, typename Visitor, typename... Variants>
constexpr decltype(auto) visit_impl(Visitor&& visitor, Variants&&... variants) {
  if constexpr (std::is_same_v<R, indexed_visit>) {
    constexpr auto& table = impl::get_table<R, void, Visitor&&, Variants&&...>::table;
    auto func_ptr = table.get_ptr(variants.index()...);
    return (*func_ptr)(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
  } else {
    constexpr auto& table = impl::get_table<void, R, Visitor&&, Variants&&...>::table;
    auto func_ptr = table.get_ptr(variants.index()...);
    return (*func_ptr)(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
  }
}

template <typename Visitor, typename... Variants>
constexpr void visit_with_index(Visitor&& visitor, Variants&&... variants) {
  visit_impl<indexed_visit>(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}
} // namespace impl

template <typename... Types>
class variant {
public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<variant_alternative_t<0, variant<Types...>>>)
    requires (std::is_default_constructible_v<variant_alternative_t<0, variant<Types...>>>)
      : data()
      , index_(0) {
    std::construct_at(const_cast<std::remove_const_t<decltype(data.first)>*>(&data.first));
  }

  constexpr variant(const variant& other) = default;

  constexpr variant(const variant& other) noexcept(impl::traits<Types...>::nothrow_copy_constr)
    requires (impl::traits<Types...>::copy_constr && !impl::traits<Types...>::trivially_copy_constr)
      : index_(other.index_) {
    if (!other.valueless_by_exception()) {
      impl::visit_with_index(
          [this]<typename T>(T&& value, auto other_index) {
            constexpr std::size_t index = other_index;
            std::construct_at(&this->data, in_place_index<index>, value);
          },
          const_cast<variant&>(other)
      );
    }
  }

  constexpr variant(variant&& other) = default;

  constexpr variant(variant&& other) noexcept(impl::traits<Types...>::nothrow_move_constr)
    requires (impl::traits<Types...>::move_constr && !impl::traits<Types...>::trivially_move_constr)
      : index_(other.index_) {
    if (!other.valueless_by_exception()) {
      impl::visit_with_index(
          [this]<typename T>(T&& value, auto other_index) {
            constexpr std::size_t index = other_index;
            std::construct_at(&this->data, in_place_index<index>, std::move(value));
          },
          const_cast<variant&>(other)
      );
    }
  }

  template <typename T, typename Chosen = impl::chose_type<T, Types...>>
    requires (sizeof...(Types) > 0 && !std::is_same_v<std::decay_t<T>, variant> && !impl::is_in_place_t<T> && std::is_constructible_v<Chosen, T>)
  constexpr variant(T&& t) noexcept(std::is_nothrow_constructible_v<Chosen, T>)
      : variant(in_place_index<impl::get_index_by_type_v<Chosen, Types...>>, std::forward<T>(t)) {}

  template <typename T, typename... Args>
    requires (std::is_constructible_v<T, Args...> && impl::only_once<T, Types...>)
  constexpr explicit variant(in_place_type_t<T>, Args&&... args)
      : variant(in_place_index<impl::get_index_by_type_v<T, Types...>>, std::forward<Args>(args)...) {}

  template <std::size_t I, typename... Args>
    requires (I < sizeof...(Types) && std::is_constructible_v<impl::get_type_by_index_t<I, Types...>, Args...>)
  constexpr explicit variant(in_place_index_t<I>, Args&&... args)
      : data(in_place_index<I>, std::forward<Args>(args)...)
      , index_(I) {}

  constexpr void clear() {
    if (!valueless_by_exception()) {
      visit([]<typename T>(T&& value) { std::destroy_at(std::addressof(value)); }, *this);
      index_ = variant_npos;
    }
  }

  constexpr ~variant() = default;

  constexpr ~variant()
    requires (!impl::traits<Types...>::trivially_destruct)
  {
    clear();
  }

  constexpr variant& operator=(const variant& other) = default;

  constexpr variant& operator=(const variant& other)
    requires (
        impl::traits<Types...>::copy_constr && impl::traits<Types...>::copy_assign &&
        !impl::traits<Types...>::trivially_copy_assign
    )
  {
    if (other.valueless_by_exception()) {
      clear();
    } else {
      impl::visit_with_index(
          [this, &other]<typename T>(T&& value, auto other_index) {
            using type = std::remove_reference_t<T>;
            constexpr std::size_t index = other_index;
            if (this->index() == index) {
              get<index>(*this) = value;
            } else if constexpr (impl::traits<type>::nothrow_copy_constr || !impl::traits<type>::nothrow_move_constr) {
              this->emplace<index>(value);
            } else {
              this->operator=(variant(other));
            }
          },
          const_cast<variant&>(other)
      );
    }
    return *this;
  }

  constexpr variant& operator=(variant&& other) = default;

  constexpr variant& operator=(variant&& other
  ) noexcept(impl::traits<Types...>::nothrow_move_constr && impl::traits<Types...>::nothrow_move_assign)
    requires (
        impl::traits<Types...>::move_assign && impl::traits<Types...>::move_constr &&
        !impl::traits<Types...>::trivially_move_assign
    )
  {
    if (other.valueless_by_exception()) {
      clear();
    } else {
      impl::visit_with_index(
          [this, &other]<typename T>(T&& value, auto other_index) {
            constexpr std::size_t index = other_index;
            if (this->index() == index) {
              get<index>(*this) = std::move(value);
            } else {
              this->emplace<index>(std::move(*get_if<index>(std::addressof(other))));
            }
          },
          other
      );
    }
    return *this;
  }

  template <typename T, typename Chosen = impl::chose_type<T, Types...>>
  constexpr variant& operator=(T&& t
  ) noexcept(std::is_nothrow_assignable_v<Chosen&, T> && std::is_nothrow_constructible_v<Chosen, T>)
    requires (impl::only_once<Chosen, Types...> && !std::is_same_v<std::decay_t<T>, variant> && std::is_constructible_v<Chosen, T> && std::is_assignable_v<Chosen&, T>)
  {
    constexpr std::size_t index = impl::get_index_by_type_v<Chosen, Types...>;
    if (index_ == index) {
      get<index>(*this) = std::forward<T>(t);
    } else if constexpr (std::is_nothrow_constructible_v<Chosen, T> || !std::is_nothrow_move_constructible_v<Chosen>) {
      emplace<index>(std::forward<T>(t));
    } else {
      emplace<index>(Chosen(std::forward<T>(t)));
    }
    return *this;
  }

  template <typename T, typename... Args>
  constexpr T& emplace(Args&&... args)
    requires (std::is_constructible_v<T, Args...> && impl::only_once<T, Types...>)
  {
    constexpr std::size_t index = impl::get_index_by_type_v<T, Types...>;
    return this->emplace<index>(std::forward<Args>(args)...);
  }

  template <std::size_t I, typename... Args>
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args)
    requires (std::is_constructible_v<impl::get_type_by_index_t<I, Types...>, Args...> && I < sizeof...(Types))
  {
    clear();
    std::construct_at(&data, in_place_index<I>, std::forward<Args>(args)...);
    index_ = I;
    return get<I>(*this);
  }

  constexpr void swap(variant& other
  ) noexcept(impl::traits<Types...>::nothrow_move_constr && impl::traits<Types...>::nothrow_swappable) {
    if (other.valueless_by_exception()) {
      if (!valueless_by_exception()) {
        other.swap(*this);
      }
    } else {
      if (valueless_by_exception()) {
        visit(
            [this, &other]<typename T>(T&& value) {
              using type = std::remove_reference_t<T>;
              this->emplace<type>(std::move(value));
              other.clear();
            },
            other
        );
      } else {
        visit(
            [this, &other]<typename T1, typename T2>(T1&& value, T2&& this_value) {
              using type = std::remove_reference_t<T1>;
              constexpr std::size_t index = impl::get_index_by_type_v<type, Types...>;
              using this_type = std::remove_reference_t<T2>;
              constexpr std::size_t this_index = impl::get_index_by_type_v<this_type, Types...>;
              if constexpr (index == this_index) {
                using std::swap;
                swap(this_value, value);
              } else {
                auto temp(std::move(value));
                other.emplace<this_index>(std::move(this_value));
                this->emplace<index>(std::move(temp));
              }
            },
            other,
            *this
        );
      }
    }
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index_ == variant_npos;
  }

  constexpr std::size_t index() const noexcept {
    return index_;
  }

private:
  template <size_t I, typename... Ts>
  friend constexpr variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...>& v);
  template <size_t I, typename... Ts>
  friend constexpr variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...>&& v);
  template <size_t I, typename... Ts>
  friend constexpr const variant_alternative_t<I, variant<Ts...>>& get(const variant<Ts...>& v);
  template <size_t I, typename... Ts>
  friend constexpr const variant_alternative_t<I, variant<Ts...>>&& get(const variant<Ts...>&& v);

  impl::variadic_union<Types...> data;
  std::size_t index_;
};

template <typename... Types>
  requires (impl::traits<Types...>::move_constr && impl::traits<Types...>::swappable)
constexpr void swap(variant<Types...>& lhs, variant<Types...>& rhs) noexcept(
    impl::traits<Types...>::nothrow_move_constr && impl::traits<Types...>::nothrow_swappable
) {
  lhs.swap(rhs);
}

template <typename... Types>
  requires (!impl::traits<Types...>::move_constr || !impl::traits<Types...>::swappable)
constexpr void swap(variant<Types...>&, variant<Types...>&) = delete;

template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept
  requires (impl::only_once<T, Types...>)
{
  return impl::get_index_by_type_v<T, Types...> == v.index();
}

template <std::size_t I, typename... Types>
constexpr variant_alternative_t<I, variant<Types...>>* get_if(variant<Types...>* pv) noexcept
  requires (I < sizeof...(Types))
{
  if (pv && pv->index() == I) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <std::size_t I, typename... Types>
constexpr const variant_alternative_t<I, variant<Types...>>* get_if(const variant<Types...>* pv) noexcept
  requires (I < sizeof...(Types))
{
  if (pv && pv->index() == I) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <typename T, typename... Types>
constexpr T* get_if(variant<Types...>* pv) noexcept
  requires (impl::only_once<T, Types...>)
{
  constexpr std::size_t index = impl::get_index_by_type_v<T, Types...>;
  return get_if<index>(pv);
}

template <typename T, typename... Types>
constexpr const T* get_if(const variant<Types...>* pv) noexcept
  requires (impl::only_once<T, Types...>)
{
  constexpr std::size_t index = impl::get_index_by_type_v<T, Types...>;
  return get_if<index>(pv);
}

template <typename... Types>
constexpr auto&& as_variant(variant<Types...>& variant) noexcept {
  return variant;
}

template <typename... Types>
constexpr auto&& as_variant(const variant<Types...>& variant) noexcept {
  return variant;
}

template <typename... Types>
constexpr auto&& as_variant(variant<Types...>&& variant) noexcept {
  return std::move(variant);
}

template <typename... Types>
constexpr auto&& as_variant(const variant<Types...>&& variant) noexcept {
  return std::move(variant);
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... variants) {
  if ((as_variant(variants).valueless_by_exception() || ...)) {
    throw bad_variant_access{};
  }
  using R = std::invoke_result_t<Visitor, decltype(get<0>(std::declval<Variants>()))...>;
  return impl::visit_impl<R>(std::forward<Visitor>(visitor), as_variant(std::forward<Variants>(variants))...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& visitor, Variants&&... variants) {
  if ((as_variant(variants).valueless_by_exception() || ...)) {
    throw bad_variant_access{};
  }
  return impl::visit_impl<R>(std::forward<Visitor>(visitor), as_variant(std::forward<Variants>(variants))...);
}

template <typename... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a == b; });
}

template <typename... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a != b; });
}

template <typename... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a < b; });
}

template <typename... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a > b; });
}

template <typename... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a <= b; });
}

template <typename... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  return compare_impl(v, w, [](auto&& a, auto&& b) { return a >= b; });
}

template <typename... Types, typename Compare>
constexpr bool compare_impl(const variant<Types...>& v, const variant<Types...>& w, Compare&& comp) {
  if (v.index() != w.index() || v.valueless_by_exception()) {
    return comp(v.index() + 1, w.index() + 1);
  } else {
    return visit(
        [&comp](auto&& value1, auto&& value2) {
          using type1 = std::remove_cvref_t<decltype(value1)>;
          using type2 = std::remove_cvref_t<decltype(value2)>;
          if constexpr (std::is_same_v<type1, type2>) {
            return comp(value1, value2);
          } else {
            return false;
          }
        },
        v,
        w
    );
  }
}

template <typename... Types>
constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>
operator<=>(const variant<Types...>& v, const variant<Types...>& w) {
  using type = std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>;
  if (v.index() != w.index() || v.valueless_by_exception()) {
    return (v.index() + 1 <=> w.index() + 1);
  } else {
    return visit(
        [](auto&& value1, auto&& value2) -> type {
          using type1 = std::remove_cvref_t<decltype(value1)>;
          using type2 = std::remove_cvref_t<decltype(value2)>;
          if constexpr (std::is_same_v<type1, type2>) {
            return (value1 <=> value2);
          } else {
            return type::equivalent;
          }
        },
        v,
        w
    );
  }
}
