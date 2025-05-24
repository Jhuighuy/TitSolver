/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <ctime>
#include <format>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/tuple.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String-like type.
template<class Str>
concept str_like = std::is_object_v<Str> && //
                   std::constructible_from<std::string_view, Str>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zero-terminated string view.
class CStrView final : public std::string_view {
public:

  /// Construct a zero-terminated string view from a string literal.
  template<size_t Size>
    requires (Size > 0)
  consteval explicit(false) CStrView(carr_ref_t<const char, Size> str) noexcept
      : std::string_view{static_cast<const char*>(str), Size - 1} {
    TIT_ASSERT(str[Size - 1] == '\0', "String is not zero-terminated!");
  }

  /// Construct a zero-terminated string view from a raw pointer.
  constexpr explicit CStrView(const char* str) noexcept
      : std::string_view{str} {
    TIT_ASSERT(str != nullptr, "String is null!");
  }

  /// Construct a zero-terminated string view from a string.
  constexpr explicit(false) CStrView(const std::string& str) noexcept
      : std::string_view{str} {}

  /// Get the underlying zero-terminated string.
  constexpr auto c_str() const noexcept -> const char* {
    return data();
  }

}; // class CStrView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String hash function.
struct StrHash final {
  using is_transparent = void;
  constexpr auto operator()(std::string_view str) const noexcept -> size_t {
    constexpr std::hash<std::string_view> hasher{};
    return hasher(str);
  }
};

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

template<std::ranges::input_range Strs>
  requires str_like<std::ranges::range_reference_t<Strs>>
auto str_join(std::string_view sep, Strs&& strs) -> std::string {
  TIT_ASSUME_UNIVERSAL(Strs, strs);
  std::string result{};
  for (const auto& str : strs) {
    if (!result.empty()) result += sep;
    if (!str.empty()) result += str;
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Add quotes to the string, if not already quoted.
inline auto str_quote(std::string_view str) -> std::string {
  if (str.size() > 1 && str.front() == '"' && str.back() == '"') {
    return std::string{str};
  }
  return std::format("\"{}\"", str);
}

/// Remove quotes from the string, if present.
auto str_unquote(std::string_view str) -> std::string_view;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Format memory size in bytes as a pretty string.
auto fmt_memsize(uint64_t value, size_t precision = 1) -> std::string;

/// Format measurement as a pretty string with SI prefix.
/// @{
auto fmt_measurement(long double value,
                     std::string_view unit,
                     size_t precision = 1) -> std::string;
template<class Val>
  requires std::integral<Val> || std::floating_point<Val>
auto fmt_measurement(Val value, std::string_view unit, size_t precision = 1)
    -> std::string {
  return fmt_measurement(static_cast<long double>(value), unit, precision);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parse a time point from a string.
[[gnu::format(strftime, 1, 0)]]
auto str_to_tm(const char* fmt, CStrView tm_str) -> std::tm;

/// Format a time point to a string.
[[gnu::format(strftime, 1, 0)]]
auto fmt_tm(const char* fmt, const std::tm& tm) -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Formatter for `CStrView`.
template<>
struct std::formatter<tit::CStrView> : std::formatter<std::string_view> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
