/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tit::sym {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exact rational number backed by checked 64-bit integers.
class Rational final {
public:

  /// Integer storage type.
  using value_type = std::int64_t;

  /// Construct zero.
  constexpr Rational() = default;

  /// Construct an integer.
  constexpr explicit(false) Rational(value_type value) noexcept
      : numerator_{value} {}

  /// Construct and normalize a fraction.
  Rational(__int128_t numerator, __int128_t denominator);

  /// Numerator.
  constexpr auto numerator() const noexcept -> value_type {
    return numerator_;
  }

  /// Positive denominator.
  constexpr auto denominator() const noexcept -> value_type {
    return denominator_;
  }

  /// Is this value an integer?
  constexpr auto is_integer() const noexcept -> bool {
    return denominator_ == 1;
  }

  /// Convert to a double-precision approximation.
  constexpr auto as_double() const noexcept -> double {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
  }

  /// Structural hash value.
  auto hash() const noexcept -> std::size_t;

  /// Unary plus operator.
  friend auto operator+(Rational value) -> Rational;

  /// Rational number addition.
  friend auto operator+(Rational left, Rational right) -> Rational;

  /// Rational number addition with assignment.
  friend auto operator+=(Rational& left, Rational right) -> Rational&;

  /// Rational number negation.
  friend auto operator-(Rational value) -> Rational;

  /// Rational number subtraction.
  friend auto operator-(Rational left, Rational right) -> Rational;

  /// Rational number subtraction with assignment.
  friend auto operator-=(Rational& left, Rational right) -> Rational&;

  /// Rational number multiplication.
  friend auto operator*(Rational left, Rational right) -> Rational;

  /// Rational number multiplication with assignment.
  friend auto operator*=(Rational& left, Rational right) -> Rational&;

  /// Rational number division.
  friend auto operator/(Rational left, Rational right) -> Rational;

  /// Rational number division with assignment.
  friend auto operator/=(Rational& left, Rational right) -> Rational&;

  /// Comparison operators.
  /// @{
  friend constexpr auto operator==(Rational left, Rational right) noexcept
      -> bool = default;
  friend auto operator<=>(Rational left, Rational right) noexcept
      -> std::strong_ordering;
  /// @}

private:

  value_type numerator_ = 0;
  value_type denominator_ = 1;

}; // class Rational

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym

template<>
struct std::hash<tit::sym::Rational> {
  auto operator()(tit::sym::Rational value) const noexcept -> std::size_t {
    return value.hash();
  }
};
