/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/sym/rational.hpp"

#include <compare>
#include <cstddef>
#include <limits>
#include <utility>

#include "tit/core/exception.hpp"

namespace tit::sym {
namespace {

using WideInt = __int128_t;
using WideUInt = __uint128_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto magnitude(WideInt value) noexcept -> WideUInt {
  if (value >= 0) return static_cast<WideUInt>(value);
  return static_cast<WideUInt>(-(value + 1)) + 1;
}

auto gcd(WideUInt left, WideUInt right) noexcept -> WideUInt {
  while (right != 0) {
    const auto remainder = left % right;
    left = right;
    right = remainder;
  }
  return left;
}

auto narrow(WideInt value) -> Rational::value_type {
  constexpr auto min = std::numeric_limits<Rational::value_type>::min();
  constexpr auto max = std::numeric_limits<Rational::value_type>::max();
  TIT_ENSURE(value >= static_cast<WideInt>(min) &&
                 value <= static_cast<WideInt>(max),
             "Rational arithmetic overflow.");
  return static_cast<Rational::value_type>(value);
}

auto normalized(WideInt numerator, WideInt denominator)
    -> std::pair<Rational::value_type, Rational::value_type> {
  TIT_ENSURE(denominator != 0, "Rational denominator must not be zero.");
  if (numerator == 0) return {0, 1};
  if (denominator < 0) {
    numerator = -numerator;
    denominator = -denominator;
  }
  const auto divisor = gcd(magnitude(numerator), magnitude(denominator));
  numerator /= static_cast<WideInt>(divisor);
  denominator /= static_cast<WideInt>(divisor);
  return {narrow(numerator), narrow(denominator)};
}

auto make_rational(WideInt numerator, WideInt denominator) -> Rational {
  const auto [num, den] = normalized(numerator, denominator);
  return Rational{num, den};
}

auto hash_combine(std::size_t seed, std::size_t value) noexcept -> std::size_t {
  return seed ^ (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Rational::Rational(value_type numerator, value_type denominator) {
  const auto [num, den] = normalized(numerator, denominator);
  numerator_ = num;
  denominator_ = den;
}

auto operator-(Rational value) -> Rational {
  return make_rational(-static_cast<WideInt>(value.numerator_),
                       value.denominator_);
}

auto operator+(Rational left, Rational right) -> Rational {
  const auto divisor =
      gcd(magnitude(left.denominator_), magnitude(right.denominator_));
  const auto left_scale =
      static_cast<WideInt>(right.denominator_) / static_cast<WideInt>(divisor);
  const auto right_scale =
      static_cast<WideInt>(left.denominator_) / static_cast<WideInt>(divisor);
  const auto numerator = static_cast<WideInt>(left.numerator_) * left_scale +
                         static_cast<WideInt>(right.numerator_) * right_scale;
  const auto denominator = static_cast<WideInt>(left.denominator_) * left_scale;
  return make_rational(numerator, denominator);
}

auto operator-(Rational left, Rational right) -> Rational {
  return left + -right;
}

auto operator*(Rational left, Rational right) -> Rational {
  const auto left_divisor =
      gcd(magnitude(left.numerator_), magnitude(right.denominator_));
  const auto right_divisor =
      gcd(magnitude(right.numerator_), magnitude(left.denominator_));
  const auto numerator = (static_cast<WideInt>(left.numerator_) /
                          static_cast<WideInt>(left_divisor)) *
                         (static_cast<WideInt>(right.numerator_) /
                          static_cast<WideInt>(right_divisor));
  const auto denominator = (static_cast<WideInt>(left.denominator_) /
                            static_cast<WideInt>(right_divisor)) *
                           (static_cast<WideInt>(right.denominator_) /
                            static_cast<WideInt>(left_divisor));
  return make_rational(numerator, denominator);
}

auto operator/(Rational left, Rational right) -> Rational {
  TIT_ENSURE(right.numerator_ != 0, "Cannot divide a rational by zero.");
  return left * make_rational(right.denominator_, right.numerator_);
}

auto Rational::operator+=(Rational other) -> Rational& {
  return *this = *this + other;
}

auto Rational::operator-=(Rational other) -> Rational& {
  return *this = *this - other;
}

auto Rational::operator*=(Rational other) -> Rational& {
  return *this = *this * other;
}

auto Rational::operator/=(Rational other) -> Rational& {
  return *this = *this / other;
}

auto operator<=>(Rational left, Rational right) noexcept
    -> std::strong_ordering {
  const auto left_value = static_cast<WideInt>(left.numerator_) *
                          static_cast<WideInt>(right.denominator_);
  const auto right_value = static_cast<WideInt>(right.numerator_) *
                           static_cast<WideInt>(left.denominator_);
  if (left_value < right_value) return std::strong_ordering::less;
  if (left_value > right_value) return std::strong_ordering::greater;
  return std::strong_ordering::equal;
}

auto Rational::hash() const noexcept -> std::size_t {
  auto result = std::hash<value_type>{}(numerator_);
  return hash_combine(result, std::hash<value_type>{}(denominator_));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
