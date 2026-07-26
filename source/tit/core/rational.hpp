/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <numeric>
#include <utility>

#include "tit/core/exception.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Rational number backed by a pair of 64-bit integers.
class Rational final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a rational number from a numerator and a denominator.
  template<std::integral Num = std::int64_t, std::integral Den = Num>
  constexpr explicit(false) Rational(Num numerator = Num{0}, //
                                     Den denominator = Den{1}) {
    TIT_ENSURE(denominator != 0, "Rational denominator must not be zero!");
    if (denominator < 0) {
      numerator = -numerator;
      denominator = -denominator;
    }
    if (const auto common = std::gcd(numerator, denominator); common > 1) {
      numerator /= common;
      denominator /= common;
    }
    num_ = narrow_(numerator);
    den_ = narrow_(denominator);
  }

  /// Numerator of the number.
  constexpr auto num() const noexcept -> std::int64_t {
    return num_;
  }

  /// Denominator of the number, always positive.
  constexpr auto den() const noexcept -> std::int64_t {
    return den_;
  }

  /// Is this number zero?
  constexpr auto is_zero() const noexcept -> bool {
    return num_ == 0;
  }

  /// Is this number an integer?
  constexpr auto is_integer() const noexcept -> bool {
    return den_ == 1;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Rational number unary plus operator.
  friend constexpr auto operator+(const Rational& a) -> Rational {
    return a;
  }

  /// Rational number addition.
  friend constexpr auto operator+(const Rational& a, const Rational& b)
      -> Rational {
    const auto common = std::gcd(Wide{a.den()}, Wide{b.den()});
    const auto a_scale = Wide{b.den()} / common;
    const auto b_scale = Wide{a.den()} / common;
    return {Wide{a.num()} * a_scale + Wide{b.num()} * b_scale,
            Wide{a.den()} * a_scale};
  }

  /// Rational number addition with assignment.
  friend constexpr auto operator+=(Rational& a, const Rational& b)
      -> Rational& {
    a = a + b;
    return a;
  }

  /// Rational number negation.
  friend constexpr auto operator-(const Rational& a) -> Rational {
    return {-Wide{a.num()}, a.den()};
  }

  /// Rational number subtraction.
  friend constexpr auto operator-(const Rational& a, const Rational& b)
      -> Rational {
    return a + (-b);
  }

  /// Rational number subtraction with assignment.
  friend constexpr auto operator-=(Rational& a, const Rational& b)
      -> Rational& {
    a = a - b;
    return a;
  }

  /// Rational number multiplication.
  friend constexpr auto operator*(const Rational& a, const Rational& b)
      -> Rational {
    const auto left = std::gcd(Wide{a.num()}, Wide{b.den()});
    const auto right = std::gcd(Wide{b.num()}, Wide{a.den()});
    return {(Wide{a.num()} / left) * (Wide{b.num()} / right),
            (Wide{a.den()} / right) * (Wide{b.den()} / left)};
  }

  /// Rational number multiplication with assignment.
  friend constexpr auto operator*=(Rational& a, const Rational& b)
      -> Rational& {
    a = a * b;
    return a;
  }

  /// Rational number division.
  friend constexpr auto operator/(const Rational& a, const Rational& b)
      -> Rational {
    TIT_ENSURE(!b.is_zero(), "Rational division by zero!");
    return a * Rational{b.den(), b.num()};
  }

  /// Rational number division with assignment.
  friend constexpr auto operator/=(Rational& a, const Rational& b)
      -> Rational& {
    a = a / b;
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Equality operator.
  friend constexpr auto operator==(const Rational& a, const Rational& b)
      -> bool {
    return a.num() == b.num() && a.den() == b.den();
  }

  /// Comparison operator.
  friend constexpr auto operator<=>(const Rational& a, const Rational& b)
      -> std::strong_ordering {
    return Wide{a.num()} * Wide{b.den()} <=> Wide{b.num()} * Wide{a.den()};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Double-width integer used for the intermediate results.
  using Wide = __int128_t;

  // Narrow a wide integer back to the storage type.
  static constexpr auto narrow_(Wide value) -> std::int64_t {
    TIT_ENSURE(std::in_range<std::int64_t>(value), "Rational overflow!");
    return static_cast<std::int64_t>(value);
  }

  std::int64_t num_ = 0;
  std::int64_t den_ = 1;

}; // class Rational

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
