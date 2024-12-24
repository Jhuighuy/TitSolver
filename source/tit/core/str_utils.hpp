/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstring>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zero-terminated string view.
class CStrView final {
public:

  /// Construct a zero-terminated string view.
  /// @{
  constexpr CStrView() noexcept : str_{""} {}
  constexpr explicit(false) CStrView(const char* str) noexcept
      : str_{str != nullptr ? str : ""} {}
  constexpr explicit(false) CStrView(const std::string& str) noexcept
      : str_{str.c_str()} {}
  /// @}

  /// Cast to `std::string`.
  constexpr explicit operator std::string() const {
    return std::string{c_str()};
  }

  /// Cast to `std::string_view`.
  constexpr explicit(false) operator std::string_view() const noexcept {
    return std::string_view{c_str()};
  }

  /// Get the underlying string.
  constexpr auto c_str() const noexcept -> const char* {
    TIT_ASSERT(str_ != nullptr, "String is null!");
    return str_;
  }

  /// Check if the string is empty.
  constexpr auto empty() const noexcept -> bool {
    return *c_str() == '\0';
  }

  /// Size of the string.
  constexpr auto size() const noexcept -> size_t {
    return std::strlen(c_str());
  }

  /// Get the character at the given index.
  constexpr auto operator[](size_t i) const noexcept -> char {
    TIT_ASSERT(i < size(), "Index is out of range!");
    return c_str()[i]; // NOLINT(*-bounds-pointer-arithmetic)
  }

  /// Compare two strings by value.
  friend constexpr auto operator==(CStrView lhs, CStrView rhs) noexcept
      -> bool {
    return std::strcmp(lhs.c_str(), rhs.c_str()) == 0;
  }

private:

  const char* str_;

}; // class CStrView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String hash function.
struct StrHash final {
  using is_transparent = void;
  auto operator()(std::string_view str) const noexcept -> size_t {
    constexpr std::hash<std::string_view> hasher{};
    return hasher(str);
  }
}; // struct StrHash

/// String hash set.
using StrHashSet = std::unordered_set<std::string, StrHash, std::equal_to<>>;

/// String hash map.
template<class Val>
using StrHashMap =
    std::unordered_map<std::string, Val, StrHash, std::equal_to<>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String case-insensitive comparison.
struct StrNocaseEqual final {
  using is_transparent = void;
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

} // namespace tit

// Formatter for `CStrView`.
template<>
struct std::formatter<tit::CStrView> : std::formatter<std::string_view> {
  constexpr auto format(const tit::CStrView& str, auto& context) const {
    return std::formatter<std::string_view>::format(str.c_str(), context);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
