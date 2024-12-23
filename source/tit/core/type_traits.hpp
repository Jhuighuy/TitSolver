/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that value @p X is in range [ @p A, @p B ].
template<auto X, decltype(X) A, decltype(X) B>
inline constexpr bool in_range_v = A <= X && X <= B;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the template type can be constructed from the given arguments.
template<template<class...> class T, class... Args>
concept deduce_constructible_from = requires(Args... args) { T{args...}; };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Contiguous range with fixed size.
template<class Range>
concept contiguous_fixed_size_range =
    requires(Range& range) { std::span{range}; } && //
    decltype(std::span{std::declval<Range&>()})::extent != std::dynamic_extent;

/// Size of the contiguous fixed size range.
template<contiguous_fixed_size_range Range>
inline constexpr auto range_fixed_size_v =
    decltype(std::span{std::declval<Range&>()})::extent;

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

namespace impl {

template<class Type, size_t I>
concept has_tuple_element = requires {
  typename std::tuple_element_t<I, std::remove_const_t<Type>>;
} && requires(Type t) {
  {
    std::get<I>(t)
  } -> std::convertible_to<const std::tuple_element_t<I, Type>&>;
};

} // namespace impl

/// Tuple-like type.
template<class Type>
concept tuple_like = !std::is_reference_v<Type> && requires(Type t) {
  typename std::tuple_size<Type>::type;
  requires std::derived_from<
      std::tuple_size<Type>,
      std::integral_constant<size_t, std::tuple_size_v<Type>>>;
} && []<size_t... I>(std::index_sequence<I...>) {
  return (impl::has_tuple_element<Type, I> && ...);
}(std::make_index_sequence<std::tuple_size_v<Type>>());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that type `T` is in the list `Us...`.
template<class T, class... Us>
inline constexpr bool contains_v = (... || std::is_same_v<T, Us>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<size_t I, class... Ts>
struct type_at;
template<class T, class... Ts>
struct type_at<0, T, Ts...> : std::type_identity<T> {};
template<size_t I, class T, class... Ts>
struct type_at<I, T, Ts...> : type_at<I - 1, Ts...> {};

} // namespace impl

/// Type at position `I` in list `Ts...`.
/// @todo Replace with C++26's pack indexing when available.
template<size_t I, class... Ts>
  requires (I < sizeof...(Ts))
using type_at_t = typename impl::type_at<I, Ts...>::type;

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

/// Negation result type.
template<class Num>
using negate_result_t = decltype(-std::declval<Num>());

/// Addition result type.
template<class NumA, class NumB = NumA>
using add_result_t = decltype(std::declval<NumA>() + std::declval<NumB>());

/// Subtraction result type.
template<class NumA, class NumB = NumA>
using sub_result_t = decltype(std::declval<NumA>() - std::declval<NumB>());

/// Multiplication result type.
template<class NumA, class NumB = NumA>
using mul_result_t = decltype(std::declval<NumA>() * std::declval<NumB>());

/// Division result type.
template<class NumA, class NumB = NumA>
using div_result_t = decltype(std::declval<NumA>() / std::declval<NumB>());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compile-time value constant.
template<auto Val>
struct value_constant_t {
  static constexpr auto value = Val;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
