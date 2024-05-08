/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/basic_types.hpp"

namespace tit::meta {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// An empty and trivial object.
template<class T>
concept type =
    std::is_object_v<T> && std::is_empty_v<T> && std::is_trivial_v<T>;

/// Check that type `T` is in the list `Us...`.
template<type T, type... Us>
inline constexpr bool contains_v = (... || std::is_same_v<T, Us>);

namespace impl {

template<class T, class U, class... Us>
inline constexpr size_t index_of_v = 1 + index_of_v<T, Us...>;
template<class T, class... Us>
inline constexpr size_t index_of_v<T, T, Us...> = 0;

template<class... Ts>
inline constexpr bool all_unique_v = true;
template<class T, class... Ts>
inline constexpr bool all_unique_v<T, Ts...> =
    (!contains_v<T, Ts...> && all_unique_v<Ts...>);

} // namespace impl

/// Index of type `T` in list `Us...`.
template<type T, type... Us>
  requires contains_v<T, Us...>
inline constexpr auto index_of_v = impl::index_of_v<T, Us...>;

/// Check that all elements in the list `Ts...` are unique.
template<class... Ts>
inline constexpr bool all_unique_v = impl::all_unique_v<Ts...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set of empty types, with no duplicates.
template<type... Ts>
  requires all_unique_v<Ts...>
class Set {
public:

  /// Construct a set.
  /// @{
  consteval Set() noexcept = default;
  consteval explicit Set(Ts... /*elems*/)
    requires (sizeof...(Ts) != 0)
  {}
  /// @}

  /// Check if the set contains a type `U`.
  template<type U>
  constexpr auto contains(U /*elem*/) const noexcept -> bool {
    return contains_v<U, Ts...>;
  }

  /// Check if the set contains all types in the set `Us...`.
  template<type... Us>
  constexpr auto includes(Set<Us...> /*set*/) const noexcept -> bool {
    return (contains_v<Us, Ts...> && ...);
  }

}; // class Set

template<class T>
inline constexpr bool is_set_v = false;
template<class... Ts>
inline constexpr bool is_set_v<Set<Ts...>> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that @p lhs and @p rhs contain same elements.
template<type... Ts, type... Us>
constexpr auto operator==(Set<Ts...> lhs, Set<Us...> rhs) noexcept -> bool {
  return rhs.includes(lhs) && lhs.includes(rhs);
}

/// Check that @p lhs is a @b strict subset of RHS.
template<type... Ts, type... Us>
constexpr auto operator<(Set<Ts...> lhs, Set<Us...> rhs) noexcept -> bool {
  return rhs.includes(lhs) && (sizeof...(Ts) < sizeof...(Us));
}

/// Check that @p lhs is a subset of @p rhs.
template<type... Ts, type... Us>
constexpr auto operator<=(Set<Ts...> lhs, Set<Us...> rhs) noexcept -> bool {
  return rhs.includes(lhs);
}

/// Check that @p lhs is a @b strict superset of @p rhs.
template<type... Ts, type... Us>
constexpr auto operator>(Set<Ts...> lhs, Set<Us...> rhs) noexcept -> bool {
  return rhs < lhs;
}

/// Check that @p lhs is a superset of @p rhs.
template<type... Ts, type... Us>
constexpr auto operator>=(Set<Ts...> lhs, Set<Us...> rhs) noexcept -> bool {
  return rhs <= lhs;
}

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

template<class... Us>
constexpr auto set_and(Set<> lhs, Set<Us...> /*rhs*/) noexcept {
  return lhs;
}
template<class T, class... Ts, class... Us>
constexpr auto set_and(Set<T, Ts...> /*lhs*/, Set<Us...> rhs) noexcept {
  auto const remaining = set_and(Set<Ts...>{}, rhs);
  if constexpr (contains_v<T, Us...>) return set_or(Set<T>{}, remaining);
  else return remaining;
}

template<class... Us>
constexpr auto set_diff(Set<> lhs, Set<Us...> /*rhs*/) noexcept {
  return lhs;
}
template<class T, class... Ts, class... Us>
constexpr auto set_diff(Set<T, Ts...> /*lhs*/, Set<Us...> rhs) noexcept {
  auto const remaining = set_diff(Set<Ts...>{}, rhs);
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
///          in @p rhs. The relative order of the elements in LHS is preserved.
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

namespace impl {

template<class T>
consteval auto type_name() {
  // This thing is incomplete. It is a stub for the future set sorting.
  return __PRETTY_FUNCTION__;
}

} // namespace impl

/// Get the name of a type in compile-time.
template<class T>
inline constexpr auto type_name_v = impl::type_name<T>();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::meta
