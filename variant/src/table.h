#pragma once

#include "traits.h"

#include <functional>
#include <utility>

namespace impl {
template <typename T, std::size_t Index>
struct func_table;

template <typename T>
struct func_table<T, 0> {
  constexpr auto& get_ptr() const {
    return data;
  }

  T data;
};

template <typename Res, typename Visitor, typename... Variants, std::size_t Index>
  requires (Index > 0)
struct func_table<Res (*)(Visitor, Variants...), Index> {
  using func = Res (*)(Visitor, Variants...);
  static constexpr std::size_t size = variant_size_v<get_type_by_index_t<Index - 1, std::remove_cvref_t<Variants>...>>;

  template <typename... Args>
  constexpr decltype(auto) get_ptr(std::size_t index, Args... rest) const {
    return data[index].get_ptr(rest...);
  }

  func_table<func, Index - 1> data[size];
};

template <typename Special, typename T, typename Seq>
struct get_table_impl;

template <
    typename Special,
    typename R,
    typename Visitor,
    std::size_t TableIndex,
    typename... Variants,
    std::size_t... Indices>
struct get_table_impl<Special, func_table<R (*)(Visitor, Variants...), TableIndex>, std::index_sequence<Indices...>> {
public:
  using next = std::remove_reference_t<get_type_by_index_t<sizeof...(Indices), Variants...>>;
  using f_table = func_table<R (*)(Visitor, Variants...), TableIndex>;

  static constexpr f_table create() {
    f_table table{};
    create_alternatives(table, std::make_index_sequence<variant_size_v<next>>{});
    return table;
  }

private:
  template <size_t... VariantIndices>
  static constexpr void create_alternatives(f_table& table, std::index_sequence<VariantIndices...>) {
    (create_alternative<VariantIndices>(table.data[VariantIndices]), ...);
  }

  template <size_t Index, typename T>
  static constexpr void create_alternative(T& el) {
    el = get_table_impl<Special, std::remove_reference_t<decltype(el)>, std::index_sequence<Indices..., Index>>::create(
    );
  }
};

template <typename Special, typename Res, typename Visitor, typename... Variants, size_t... Indices>
struct get_table_impl<Special, func_table<Res (*)(Visitor, Variants...), 0>, std::index_sequence<Indices...>> {
public:
  using f_table = func_table<Res (*)(Visitor, Variants...), 0>;

  static constexpr f_table create() {
    return f_table{&invoke_visitor};
  }

private:
  static constexpr decltype(auto) invoke_visitor(Visitor&& visitor, Variants&&... variants) {
    if constexpr (std::is_same_v<Special, indexed_visit>) {
      return std::invoke(
          std::forward<Visitor>(visitor),
          get<Indices>(std::forward<Variants>(variants))...,
          std::integral_constant<std::size_t, Indices>()...
      );
    } else {
      if constexpr (std::is_same_v<Res, void>) {
        return std::invoke(std::forward<Visitor>(visitor), get<Indices>(std::forward<Variants>(variants))...);
      } else {
        return static_cast<Res>(
            std::invoke(std::forward<Visitor>(visitor), get<Indices>(std::forward<Variants>(variants))...)
        );
      }
    }
  }
};

template <typename Special, typename Res, typename Visitor, typename... Variants>
struct get_table {
  using f_table = func_table<Res (*)(Visitor, Variants...), sizeof...(Variants)>;
  static constexpr f_table table = get_table_impl<Special, f_table, std::index_sequence<>>::create();
};
} // namespace impl
