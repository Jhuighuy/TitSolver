/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <functional>
#include <tuple>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Convert macro argument into a string literal.
#define TIT_STR(a) TIT_STR_IMPL(a)
#define TIT_STR_IMPL(a) #a

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(TIT_CAT(prefix, _), __LINE__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Use this function to assume forwarding references as universal references
/// to avoid false alarms from analysis tools.
/// @{
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))
#define TIT_ASSUME_UNIVERSALS(Ts, refs) (TIT_ASSUME_UNIVERSAL(Ts, refs), ...)
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pack values into a padded array of given size.
template<size_t Size, class T, class... Ts>
  requires ((std::constructible_from<T, Ts &&> && ...) &&
            ((sizeof...(Ts) == Size) ||
             ((sizeof...(Ts) < Size) && std::default_initializable<T>) ))
constexpr auto make_array(Ts&&... vals) -> std::array<T, Size> {
  return {T(std::forward<Ts>(vals))...};
}

/// Fill an array of the given size initialized with the given value.
template<size_t Size, class T>
  requires std::copy_constructible<T>
constexpr auto fill_array(const T& val) -> std::array<T, Size> {
  const auto get_val = [&val](auto /*arg*/) -> const T& { return val; };
  return [&get_val]<size_t... Indices>(std::index_sequence<Indices...>) {
    return std::array<T, Size>{get_val(Indices)...};
  }(std::make_index_sequence<Size>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Std {
/// Simple implementation of `std::bind_back`, since Clang 18 cannot handle
/// it's libstdc++ implementation.
/// @todo To be removed when Clang 19 is released.
template<class Func, class... BackArgs>
constexpr auto bind_back(Func&& func, BackArgs&&... back_arguments) {
  return
      [func = std::forward<Func>(func),
       back_args_tuple = std::tuple{std::forward<BackArgs>(
           back_arguments)...}]<class... FrontArgs>(FrontArgs&&... front_args) {
        return std::apply(
            [&func, &front_args...](auto&... back_args) {
              return std::invoke(func,
                                 std::forward<FrontArgs>(front_args)...,
                                 back_args...);
            },
            back_args_tuple);
      };
}
} // namespace Std

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
