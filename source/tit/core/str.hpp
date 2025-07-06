/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/tuple.hpp"

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
struct StrHash final : std::hash<std::string_view> {
  using is_transparent = void;
};

/// Hash the string.
inline constexpr StrHash str_hash{};

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

/// Format memory size in bytes as a pretty string.
auto fmt_memsize(uint64_t value, size_t precision = 1) -> std::string;

/// Format quantity as a pretty string with SI prefix.
/// @{
auto fmt_quantity(long double value,
                  std::string_view unit,
                  size_t precision = 1) -> std::string;
template<class Val>
  requires std::integral<Val> || std::floating_point<Val>
auto fmt_quantity(Val value, std::string_view unit, size_t precision = 1)
    -> std::string {
  return fmt_quantity(static_cast<long double>(value), unit, precision);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Formatter for `CStrView`.
template<>
struct std::formatter<tit::CStrView> : std::formatter<std::string_view> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
