/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
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

/// Helper object to store a string literal for non-type template parameters.
template<size_t Size>
class StringLiteral final {
public:

  /// String data.
  // NOLINTNEXTLINE(*-non-private-member-variables-in-classes)
  std::array<char, Size> data;

  /// Construct the string literal from a character array.
  consteval explicit(false) StringLiteral(carr_ref_t<const char, Size> str) {
    std::copy_n(str, Size, data.begin());
  }

}; // class StringLiteral

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

/// String to integer converter.
struct StrToInt {
  constexpr auto operator()(std::string_view str) const noexcept
      -> std::optional<int64_t> {
    /// @todo Clang-tidy segfaults if this is a `static constexpr` function.
    int64_t value = 0;
    if (const auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value);
        ec == std::errc{} && ptr == str.data() + str.size()) {
      return value;
    }
    return std::nullopt;
  }
};

/// Convert a string to an integer value.
inline constexpr StrToInt str_to_int{};

/// String to floating-point converter.
struct StrToFloat {
  constexpr auto operator()(std::string_view str) const noexcept
      -> std::optional<float64_t> {
    /// @todo Clang-tidy segfaults if this is a `static constexpr` function.
    float64_t value = 0.0;
    if (const auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value);
        ec == std::errc{} && ptr == str.data() + str.size()) {
      return value;
    }
    return std::nullopt;
  }
};

/// Convert a string to a floating-point value.
inline constexpr StrToFloat str_to_float{};

/// String to boolean converter.
struct StrToBool {
  /// @todo Clang-tidy segfaults if this is a `static constexpr` function.
  constexpr auto operator()(std::string_view str) const noexcept
      -> std::optional<bool> {
    if (str_nocase_equal(str, "true")) return true;
    if (str_nocase_equal(str, "false")) return false;
    return str_to_int(str).transform([](int64_t value) { return value != 0; });
  }
};

/// Convert a string to a boolean value.
inline constexpr StrToBool str_to_bool{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
