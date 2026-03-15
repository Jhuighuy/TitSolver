/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String hash function.
struct StrHash final : std::hash<std::string_view> {
  using is_transparent = void;
};

/// Hash the string.
inline constexpr StrHash str_hash{};

/// String hash set.
using StrHashSet = std::unordered_set<std::string, StrHash, std::equal_to<>>;

/// String hash map.
template<class Val>
using StrHashMap =
    std::unordered_map<std::string, Val, StrHash, std::equal_to<>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String case-insensitive comparison function.
struct StrNocaseEqual final {
  using is_transparent = void;
  static constexpr auto operator()(std::string_view a,
                                   std::string_view b) noexcept -> bool {
    static constexpr auto to_lower = [](char c) { return std::tolower(c); };
    return std::ranges::equal(a, b, {}, to_lower, to_lower);
  }
};

/// Compare two characters or strings case-insensitively.
inline constexpr StrNocaseEqual str_nocase_equal{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String to value converter.
template<class Val>
struct StrTo;

/// Convert a string to a value.
template<class Val>
inline constexpr StrTo<Val> str_to{};

/// String to integer or floating-point converter.
template<class Val>
  requires std::integral<Val> || std::floating_point<Val>
struct StrTo<Val> final {
  static constexpr auto operator()(std::string_view str) noexcept
      -> std::optional<Val> {
    Val value{};
    if (const auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value);
        ec == std::errc{} && ptr == str.data() + str.size()) {
      return value;
    }
    return std::nullopt;
  }
};

/// String to boolean converter.
template<>
struct StrTo<bool> final {
  static constexpr auto operator()(std::string_view str) noexcept
      -> std::optional<bool> {
    if (str_nocase_equal(str, "true")) return true;
    if (str_nocase_equal(str, "false")) return false;
    return str_to<int>(str).transform([](int value) { return value != 0; });
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Return @p str quoted and escaped for readable output.
auto str_quoted(std::string_view str) -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if @p str is a valid identifier.
auto str_is_identifier(std::string_view str) -> bool;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Split @p str by @p delim.
auto str_split(std::string_view str, char delim)
    -> std::vector<std::string_view>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Join a range of string-like values with a separator.
template<
    std::ranges::input_range Range = std::initializer_list<std::string_view>>
  requires std::convertible_to<std::ranges::range_reference_t<Range>,
                               std::string_view>
[[nodiscard]] auto str_join(Range&& range, std::string_view sep)
    -> std::string {
  std::string result;
  for (bool first = true; auto&& part : range) {
    if (!first) result += sep;
    first = false;
    result += std::string_view{part};
  }
  return result;
}

/// Join a range of string-like values with a separator.
/// Filter out empty values.
template<
    std::ranges::input_range Range = std::initializer_list<std::string_view>>
  requires std::convertible_to<std::ranges::range_reference_t<Range>,
                               std::string_view>
[[nodiscard]] auto str_join_nonempty(Range&& range, std::string_view sep)
    -> std::string {
  return str_join(range | std::views::filter([](std::string_view part) {
                    return !part.empty();
                  }),
                  sep);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Format a real number while keeping a decimal point for integral values.
auto fmt_real(float64_t value) -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Format memory size in bytes as a pretty string.
auto fmt_memsize(uint64_t value, size_t precision = 1) -> std::string;

/// Format quantity as a pretty string with SI prefix.
/// @{
auto fmt_quantity(float64_t value, std::string_view unit, size_t precision = 1)
    -> std::string;
template<class Val>
  requires std::integral<Val> || std::floating_point<Val>
auto fmt_quantity(Val value, std::string_view unit, size_t precision = 1)
    -> std::string {
  return fmt_quantity(static_cast<float64_t>(value), unit, precision);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
