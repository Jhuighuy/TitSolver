/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the two types are different.
template<class T, class U>
concept different_from =
    !std::same_as<std::remove_cv_t<T>, std::remove_cv_t<U>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the template type can be constructed from the given arguments.
template<template<class...> class T, class... Args>
concept deduce_constructible_from = requires (Args... args) { T{args...}; };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Type, template<class...> class Class>
inline constexpr bool is_specialization_of_v = false;

template<class... Args, template<class...> class Class>
inline constexpr bool is_specialization_of_v<Class<Args...>, Class> = true;

} // namespace impl

/// Check if type is a specialization of the given template.
template<class Type, template<class...> class Class>
concept specialization_of = impl::is_specialization_of_v<Type, Class>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that value @p X is in range [ @p A, @p B ].
template<auto X, decltype(X) A, decltype(X) B>
inline constexpr bool in_range_v = A <= X && X <= B;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that type `T` is in the list `Us...`.
template<class T, class... Us>
inline constexpr bool contains_v = (... || std::is_same_v<T, Us>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class T, class U, class... Us>
inline constexpr size_t index_of_v = 1 + index_of_v<T, Us...>;

template<class T, class... Us>
inline constexpr size_t index_of_v<T, T, Us...> = 0;

} // namespace impl

/// Index of type `T` in list `Us...`.
template<class T, class... Us>
  requires contains_v<T, Us...>
inline constexpr auto index_of_v = impl::index_of_v<T, Us...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class... Ts>
inline constexpr bool all_unique_v = true;

template<class T, class... Ts>
inline constexpr bool all_unique_v<T, Ts...> =
    (!contains_v<T, Ts...> && all_unique_v<Ts...>);

} // namespace impl

/// Check that all elements in the list `Ts...` are unique.
template<class... Ts>
inline constexpr bool all_unique_v = impl::all_unique_v<Ts...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Difference result type.
template<class T, class U = T>
using difference_t = decltype(std::declval<T>() - std::declval<U>());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compile-time value constant.
template<auto Val>
struct value_constant_t {
  static constexpr auto value = Val;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
