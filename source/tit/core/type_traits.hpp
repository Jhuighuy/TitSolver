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

/// Contiguous range with fixed size.
template<class Range>
concept contiguous_fixed_size_range =
    requires (Range& range) { std::span{range}; } && //
    decltype(std::span{std::declval<Range&>()})::extent != std::dynamic_extent;

/// Size of the contiguous fixed size range.
template<contiguous_fixed_size_range Range>
inline constexpr auto range_fixed_size_v =
    decltype(std::span{std::declval<Range&>()})::extent;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Tuple>
concept has_tuple_size =
    requires { typename std::tuple_size<Tuple>::type; } &&
    std::derived_from<std::tuple_size<Tuple>,
                      std::integral_constant<size_t, std::tuple_size_v<Tuple>>>;

template<class Tuple, size_t Index>
concept has_tuple_element = //
    requires {
      typename std::tuple_element_t<Index, std::remove_const_t<Tuple>>;
    } && requires (Tuple& tuple) {
      {
        std::get<Index>(tuple)
      } -> std::convertible_to<const std::tuple_element_t<Index, Tuple>&>;
    };

template<class Tuple, size_t Size>
concept tuple_size_is =
    has_tuple_size<Tuple> && (std::tuple_size_v<Tuple> == Size);

template<class Tuple, size_t Index, class Elem>
concept tuple_element_is =
    has_tuple_element<Tuple, Index> &&
    std::constructible_from<Elem, std::tuple_element_t<Index, Tuple>>;

} // namespace impl

/// Tuple-like type.
template<class Tuple>
concept tuple_like =
    std::is_object_v<Tuple> && impl::has_tuple_size<Tuple> &&
    []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
      return (impl::has_tuple_element<Tuple, Indices> && ...);
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>());

/// Tuple-like type with the items of the given types.
template<class Tuple, class... Items>
concept tuple_like_with_items =
    tuple_like<Tuple> && impl::tuple_size_is<Tuple, sizeof...(Items)> &&
    []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
      return (impl::tuple_element_is<Tuple, Indices, Items> && ...);
    }(std::make_index_sequence<sizeof...(Items)>());

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

/// Defer the type resolution of `T` based on template parameter.
template<class T, class Param>
using defer_t = std::conditional_t<std::is_same_v<T, Param>, T, T>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
