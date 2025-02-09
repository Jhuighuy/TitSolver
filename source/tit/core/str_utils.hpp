/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
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
class CStrView final : public std::string_view {
public:

  /// Construct a zero-terminated string view.
  /// @{
  constexpr CStrView(const std::string& str) noexcept : std::string_view{str} {}
  constexpr CStrView(const char* str) noexcept : std::string_view{str} {
    TIT_ASSERT(str != nullptr, "String is null!");
  }
  /// @}

  /// Get the underlying zero-terminated string.
  constexpr auto c_str() const noexcept -> const char* {
    return data();
  }

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
