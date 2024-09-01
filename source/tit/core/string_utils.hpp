/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Transparent string hash function.
struct StringHash {
  using is_transparent = void; // enables heterogeneous operations.
  auto operator()(std::string_view string) const noexcept -> size_t {
    constexpr std::hash<std::string_view> hasher{};
    return hasher(string);
  }
}; // struct StringHash

/// String hash set.
using StringHashSet =
    std::unordered_set<std::string, StringHash, std::equal_to<>>;

/// String hash map.
template<class Val>
using StringHashMap =
    std::unordered_map<std::string, Val, StringHash, std::equal_to<>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String case-insensitive comparison.
struct StrNocaseEqual {
  using is_transparent = void; // enables heterogeneous operations.
  constexpr auto operator()(char a, char b) const noexcept -> bool {
    return std::tolower(a) == std::tolower(b);
  }
  constexpr auto operator()(std::string_view a,
                            std::string_view b) const noexcept -> bool {
    return std::ranges::equal(a, b, *this);
  }
};

/// Compare two characters or strings case-insensitively.
inline constexpr StrNocaseEqual str_nocase_equal{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String to value converter.
template<class Val>
struct StrToVal {
  /// @todo Clang-tidy segfaults if this is a `static constexpr` function.
  constexpr auto operator()(std::string_view str) const noexcept
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
struct StrToVal<bool> {
  /// @todo Clang-tidy segfaults if this is a `static constexpr` function.
  constexpr auto operator()(std::string_view str) const noexcept
      -> std::optional<bool> {
    if (str_nocase_equal(str, "true")) return true;
    if (str_nocase_equal(str, "false")) return false;
    return StrToVal<int64_t>{}(str).transform(
        [](int64_t value) { return value != 0; });
  }
};

/// Convert a string to an integer value.
inline constexpr StrToVal<int64_t> str_to_int{};

/// Convert a string to an unsigned integer value.
inline constexpr StrToVal<int64_t> str_to_uint{};

/// Convert a string to a floating-point value.
inline constexpr StrToVal<float64_t> str_to_float{};

/// Convert a string to a boolean value.
inline constexpr StrToVal<bool> str_to_bool{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
