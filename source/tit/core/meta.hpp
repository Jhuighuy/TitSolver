/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/type_utils.hpp"

namespace tit::meta {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// An empty and trivial object.
template<class T>
concept type =
    std::is_object_v<T> && std::is_empty_v<T> && std::is_trivial_v<T>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set of meta types, with no duplicates.
template<type... Ts>
  requires all_unique_v<Ts...>
class Set final {
public:

  /// Construct a set.
  /// @{
  consteval Set() = default;
  consteval explicit Set(Ts... /*elems*/)
    requires (sizeof...(Ts) != 0)
  {}
  /// @}

  /// Call a function to all elements.
  template<std::invocable<Ts...> Func>
  constexpr auto apply(Func func) const -> decltype(auto) {
    return std::invoke(func, Ts{}...);
  }

  /// Call a function for each element.
  template<class Func>
    requires (std::invocable<Func, Ts> && ...)
  constexpr void for_each(Func func) const {
    (std::invoke(func, Ts{}), ...);
  }

  /// Check if the set contains a type `U`.
  template<type U>
  constexpr auto contains(U /*elem*/) const noexcept -> bool {
    return contains_v<U, Ts...>;
  }

  /// Check if the set includes a set of types `Us...`.
  template<type... Us>
  constexpr auto includes(const Set<Us...>& /*other*/) const noexcept -> bool {
    return (contains_v<Us, Ts...> && ...);
  }

  /// Index of the type in the range.
  template<type U>
    requires contains_v<U, Ts...>
  constexpr auto find(U /*elem*/) const noexcept -> size_t {
    return index_of_v<U, Ts...>;
  }

  /// Check that @p lhs and @p rhs contain same elements, in any order.
  template<type... Us>
  friend constexpr auto operator==(Set lhs, Set<Us...> rhs) noexcept -> bool {
    return rhs.includes(lhs) && lhs.includes(rhs);
  }

  /// Check that @p lhs is a @b strict subset of RHS.
  template<type... Us>
  friend constexpr auto operator<(Set lhs, Set<Us...> rhs) noexcept -> bool {
    return rhs.includes(lhs) && (sizeof...(Ts) < sizeof...(Us));
  }

  /// Check that @p lhs is a subset of @p rhs.
  template<type... Us>
  friend constexpr auto operator<=(Set lhs, Set<Us...> rhs) noexcept -> bool {
    return rhs.includes(lhs);
  }

  /// Check that @p lhs is a @b strict superset of @p rhs.
  template<type... Us>
  friend constexpr auto operator>(Set lhs, Set<Us...> rhs) noexcept -> bool {
    return rhs < lhs;
  }

  /// Check that @p lhs is a superset of @p rhs.
  template<type... Us>
  friend constexpr auto operator>=(Set lhs, Set<Us...> rhs) noexcept -> bool {
    return rhs <= lhs;
  }

}; // class Set

template<class T>
inline constexpr bool is_set_v = false;
template<class... Ts>
inline constexpr bool is_set_v<Set<Ts...>> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class... Ts>
constexpr auto set_or(Set<Ts...> lhs, Set<> /*rhs*/) noexcept {
  return lhs;
}
template<class... Ts, class U, class... Us>
constexpr auto set_or(Set<Ts...> lhs, Set<U, Us...> /*rhs*/) noexcept {
  if constexpr (contains_v<U, Ts...>) return set_or(lhs, Set<Us...>{});
  else return set_or(Set<Ts..., U>{}, Set<Us...>{});
}

template<type... Us>
constexpr auto set_and(Set<> lhs, Set<Us...> /*rhs*/) noexcept {
  return lhs;
}
template<type T, type... Ts, type... Us>
constexpr auto set_and(Set<T, Ts...> /*lhs*/, Set<Us...> rhs) noexcept {
  const auto remaining = set_and(Set<Ts...>{}, rhs);
  if constexpr (contains_v<T, Us...>) return set_or(Set<T>{}, remaining);
  else return remaining;
}

template<type... Us>
constexpr auto set_diff(Set<> lhs, Set<Us...> /*rhs*/) noexcept {
  return lhs;
}
template<type T, type... Ts, type... Us>
constexpr auto set_diff(Set<T, Ts...> /*lhs*/, Set<Us...> rhs) noexcept {
  const auto remaining = set_diff(Set<Ts...>{}, rhs);
  if constexpr (contains_v<T, Us...>) return remaining;
  else return set_or(Set<T>{}, remaining);
}

} // namespace impl

/// Set union.
///
/// @returns A set that contains all the elements of @p lhs followed by the
///          elements of @p rhs that are not already present in @p lhs. The
///          relative order of the elements in both sets is preserved.
template<type... Ts, type... Us>
constexpr auto operator|(Set<Ts...> lhs, Set<Us...> rhs) noexcept {
  return impl::set_or(lhs, rhs);
}

/// Set intersection.
///
/// @returns A set that contains the elements of @p lhs that are also present
///          in @p rhs. The relative order of the elements in LHS is
///          preserved.
template<type... Ts, type... Us>
constexpr auto operator&(Set<Ts...> lhs, Set<Us...> rhs) noexcept {
  return impl::set_and(lhs, rhs);
}

/// Set difference.
///
/// @returns A set that contains all the elements of @p lhs excluding elements
///          that are contained in @p rhs. The relative order of the elements
///          in LHS is preserved.
template<type... Ts, type... Us>
constexpr auto operator-(Set<Ts...> lhs, Set<Us...> rhs) noexcept {
  return impl::set_diff(lhs, rhs);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::meta
