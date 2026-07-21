/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/sym/rational.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Widen a 64-bit integer to a 128-bit integer.
auto widen(std::integral auto value) noexcept -> __int128_t {
  return static_cast<__int128_t>(value);
}

// Narrow a 128-bit integer to a 64-bit integer.
template<std::integral To>
auto narrow(__int128_t value) -> To {
  TIT_ENSURE(std::in_range<To>(value), "Rational arithmetic overflow.");
  return static_cast<To>(value);
}

auto hash_combine(std::size_t seed, std::size_t value) noexcept -> std::size_t {
  return seed ^ (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Rational::Rational(__int128_t numerator, __int128_t denominator) {
  TIT_ENSURE(denominator != 0, "Rational denominator must not be zero.");
  if (denominator < 0) {
    numerator = -numerator;
    denominator = -denominator;
  }
  const auto divisor = std::gcd(numerator, denominator);
  numerator_ = narrow<value_type>(numerator / divisor);
  denominator_ = narrow<value_type>(denominator / divisor);
}

auto Rational::hash() const noexcept -> std::size_t {
  constexpr std::hash<value_type> hash{};
  return hash_combine(hash(numerator_), hash(denominator_));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator+(Rational value) -> Rational {
  return value;
}

auto operator+(Rational left, Rational right) -> Rational {
  const auto divisor = std::gcd(left.denominator(), right.denominator());
  const auto left_scale = right.denominator() / divisor;
  const auto right_scale = left.denominator() / divisor;
  const auto numerator = widen(left.numerator()) * widen(left_scale) +
                         widen(right.numerator()) * widen(right_scale);
  const auto denominator = widen(left.denominator()) * widen(left_scale);
  return {numerator, denominator};
}

auto operator+=(Rational& left, Rational right) -> Rational& {
  left = left + right;
  return left;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator-(Rational value) -> Rational {
  return {-widen(value.numerator()), widen(value.denominator())};
}

auto operator-(Rational left, Rational right) -> Rational {
  return left + (-right);
}

auto operator-=(Rational& left, Rational right) -> Rational& {
  left = left - right;
  return left;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator*(Rational left, Rational right) -> Rational {
  const auto left_divisor = std::gcd(left.numerator(), right.denominator());
  const auto right_divisor = std::gcd(right.numerator(), left.denominator());
  const auto numerator = widen(left.numerator() / left_divisor) *
                         widen(right.numerator() / right_divisor);
  const auto denominator = widen(left.denominator() / right_divisor) *
                           widen(right.denominator() / left_divisor);
  return {numerator, denominator};
}

auto operator*=(Rational& left, Rational right) -> Rational& {
  left = left * right;
  return left;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator/(Rational left, Rational right) -> Rational {
  TIT_ENSURE(right.numerator() != 0, "Cannot divide a rational by zero.");
  return left * Rational{right.denominator(), right.numerator()};
}

auto operator/=(Rational& left, Rational right) -> Rational& {
  left = left / right;
  return left;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto operator<=>(Rational left, Rational right) noexcept
    -> std::strong_ordering {
  const auto left_value = widen(left.numerator()) * widen(right.denominator());
  const auto right_value = widen(right.numerator()) * widen(left.denominator());
  return left_value <=> right_value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
