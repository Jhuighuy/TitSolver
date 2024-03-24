/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(prefix, __LINE__)

/** Always inline the corresponding function. */
#define TIT_FORCE_INLINE [[gnu::always_inline]] inline

/** Never inline the corresponding function. */
#define TIT_NO_INLINE [[gnu::noinline]]

/// Use this function to assume forwarding references as universal references
/// to avoid false alarms from analysis tools.
/// @{
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))
#define TIT_ASSUME_UNIVERSALS(Ts, refs) (TIT_ASSUME_UNIVERSAL(Ts, refs), ...)
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zero initialization tag.
struct zeroinit_t {
  constexpr explicit zeroinit_t() = default;
};

/// Zero initialization tag.
inline constexpr zeroinit_t zeroinit{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pack values into a padded array of given size.
template<size_t Size, class T, class... Ts>
  requires (std::convertible_to<Ts, T> && ...) &&
           ((sizeof...(Ts) == Size) ||
            ((sizeof...(Ts) <= Size) && std::default_initializable<T>) )
constexpr auto make_array(Ts&&... vals) -> std::array<T, Size> {
  return {static_cast<T>(std::forward<Ts>(vals))...};
}

/// Fill an array of the given size initialized with the given value.
template<size_t Size, class T>
  requires std::copy_constructible<T>
constexpr auto fill_array(T const& val) -> std::array<T, Size> {
  auto const get_val = [&](auto /*arg*/) -> T const& { return val; };
  return [&]<size_t... Indices>(std::index_sequence<Indices...>) {
    return std::array<T, Size>{get_val(Indices)...};
  }(std::make_index_sequence<Size>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
