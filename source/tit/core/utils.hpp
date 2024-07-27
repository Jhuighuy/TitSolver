/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <format>
#include <iterator>
#include <ranges>
#include <string>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/string_utils.hpp"

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

/// Format a range.
/// @todo To be removed when ranges are supported in std::format.
template<std::ranges::input_range Range>
  requires std::formattable<std::ranges::range_value_t<Range>, char>
constexpr auto format_range(Range&& range) -> std::string {
  TIT_ASSUME_UNIVERSAL(Range, range);
  if (std::empty(range)) return std::string{};
  std::string result = std::format("[{}", *std::begin(range));
  for (const auto& elem : range | std::views::drop(1)) {
    result += std::format(", {}", elem);
  }
  result += "]";
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Save a value for the duration of the program per compilation unit.
///
/// Saved values are stored in static storage duration variables. The main
/// difference between this function and a static variable is that the value is
/// cached uniformly across all template instantiations. This is useful for
/// avoiding multiple definitions of the same static variable across different
/// template instantiations.
///
/// @tparam Key Unique identifier for the value. Calling this function with
///             the same key and type will return the same object per
///             compilation unit.
///
/// Example:
/// @code
/// template<class T>
/// void foo() {
///   // `static_vector` will be the same object for the duration of the,
///   // program, but different for template instantiations.
///   static std::vector<int> static_vector{};
///
///   // `saved_vector` will be the same object for the duration of the
///   // program, and the same across all template instantiations.
///   auto& saved_vector = cache_value<"saved_vector">(std::vector<int>{});
/// }
/// @endcode
template<StringLiteral Key, class Val>
  requires std::move_constructible<Val&&>
[[gnu::always_inline]]
constexpr auto save_value(Val&& val) -> Val& {
  static Val saved_val{std::forward<Val>(val)};
  return saved_val;
}

/// Helper macro to save a value for the duration of the program.
///
/// Example:
/// @code
/// template<class T>
/// void foo() {
///   // `static_vector` will be the same object for the duration of the,
///   // program, but different for template instantiations.
///   static std::vector<int> static_vector{};
///
///   // `saved_vector` will be the same object for the duration of the
///   // program, and the same across all template instantiations.
///   auto& saved_vector = TIT_SAVED_VALUE(std::vector<int>{});
/// }
/// @endcode
///
/// @see save_value
#define TIT_SAVED_VALUE(...)                                                   \
  tit::save_value<TIT_STR(TIT_NAME(__FILE__))>(__VA_ARGS__)

/// Helper macro to save a variable for the duration of the program.
///
/// Example:
/// @code
/// template<class T>
/// void foo() {
///   // `static_vector` will be the same object for the duration of the,
///   // program, but different for template instantiations.
///   static std::vector<int> static_vector{};
///
///   // `saved_vector` will be the same object for the duration of the
///   // program, and the same across all template instantiations.
///   TIT_SAVED_VARIABLE(saved_vector, std::vector<int>{});
/// }
/// @endcode
///
/// @see save_value
#define TIT_SAVED_VARIABLE(name, ...)                                          \
  auto& name = TIT_SAVED_VALUE(__VA_ARGS__) /* NOLINT(*-macro-parentheses) */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
