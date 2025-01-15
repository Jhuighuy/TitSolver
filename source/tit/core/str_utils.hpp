/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
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
#include <unordered_map>
#include <unordered_set>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/tuple_utils.hpp"

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

  /// Construct an empty string view.
  constexpr CStrView() noexcept = default;

  /// Construct a zero-terminated string view.
  /// @{
  constexpr explicit(false) CStrView(const std::string& str) noexcept
      : std::string_view{str} {}
  constexpr explicit(false) CStrView(const char* str) noexcept
      : std::string_view{str} {
    TIT_ASSERT(str != nullptr, "String is null!");
  }
  constexpr CStrView(const char* str, size_t size) noexcept
      : std::string_view{str, size} {
    // NOLINTNEXTLINE(*-pro-bounds-pointer-arithmetic)
    TIT_ASSERT(c_str()[size] == '\0', "String is not zero-terminated!");
  }
  /// @}

  /// Get the underlying zero-terminated string.
  constexpr auto c_str() const noexcept -> const char* {
    return data();
  }

}; // class CStrView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compile-time string literal.
template<size_t Size>
class StrLiteral final {
public:

  /// Construct a string literal.
  constexpr explicit(false) StrLiteral( //
      carr_ref_t<const char, Size + 1> str) noexcept
      : data_{std::to_array(str)} {}

  /// Construct a string literal from a C string view.
  constexpr explicit(false) operator CStrView() const noexcept {
    return {c_str(), size()};
  }

  /// Get the string size.
  constexpr auto size() const noexcept -> size_t {
    return Size;
  }

  /// Get the underlying zero-terminated string.
  constexpr auto c_str() const noexcept -> const char* {
    return data_.data();
  }

  // NOLINTNEXTLINE(*-non-private-member-variables-in-classes)
  std::array<char, Size + 1> data_;

}; // class StrLiteral

template<size_t SizePlusOne>
StrLiteral(carr_ref_t<const char, SizePlusOne>) -> StrLiteral<SizePlusOne - 1>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String hash function.
struct StrHash final {
  using is_transparent = void;
  constexpr auto operator()(std::string_view str) const noexcept -> size_t {
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
struct std::formatter<tit::CStrView> : std::formatter<std::string_view> {};

// Formatter for `StrLiteral`.
template<tit::size_t Size>
struct std::formatter<tit::StrLiteral<Size>> : std::formatter<tit::CStrView> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
