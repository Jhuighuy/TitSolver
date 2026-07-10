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
  constexpr explicit(false) Rational(value_type value) : numerator_{value} {}

  /// Construct and normalize a fraction.
  Rational(value_type numerator, value_type denominator);

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

  /// Arithmetic operators.
  /// @{
  friend auto operator-(Rational value) -> Rational;
  friend auto operator+(Rational left, Rational right) -> Rational;
  friend auto operator-(Rational left, Rational right) -> Rational;
  friend auto operator*(Rational left, Rational right) -> Rational;
  friend auto operator/(Rational left, Rational right) -> Rational;
  auto operator+=(Rational other) -> Rational&;
  auto operator-=(Rational other) -> Rational&;
  auto operator*=(Rational other) -> Rational&;
  auto operator/=(Rational other) -> Rational&;
  /// @}

  /// Comparison operators.
  /// @{
  friend constexpr auto operator==(Rational, Rational) noexcept
      -> bool = default;
  friend auto operator<=>(Rational left, Rational right) noexcept
      -> std::strong_ordering;
  /// @}

  /// Structural hash value.
  auto hash() const noexcept -> std::size_t;

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
