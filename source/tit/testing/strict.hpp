/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>

#include "tit/core/math.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrapper for the numerical type. Use is to prevent explicit specializations
/// for the built-in numerical types.
template<class Num>
class Strict final {
public:

  /// Construct a number.
  constexpr Strict() noexcept = default;

  /// Initialize a number with a built-in numerical value.
  constexpr explicit Strict(Num val) : val_{val} {}

  /// Cast number to a different type.
  template<class To>
  constexpr explicit operator To() const noexcept {
    return static_cast<To>(val_);
  }

  /// Compare two numbers by value.
  constexpr auto operator<=>(const Strict&) const noexcept = default;

  /// Get the underlying value.
  /// @{
  constexpr auto get() noexcept -> Num& {
    return val_;
  }
  constexpr auto get() const noexcept -> const Num& {
    return val_;
  }
  /// @}

private:

  Num val_;

}; // class Strict

template<std::floating_point Real>
constexpr auto tiny_number_v<Strict<Real>> = Strict{tiny_number_v<Real>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number literal with a `float` underlying type.
constexpr auto operator""_f(long double val) noexcept {
  return Strict{static_cast<float>(val)};
}

/// Number literal with a `double` underlying type.
constexpr auto operator""_d(long double val) noexcept {
  return Strict{static_cast<double>(val)};
}

/// Number literal with a `long double` underlying type.
constexpr auto operator""_ld(long double val) noexcept {
  return Strict{val};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number unary plus operator.
template<class Num>
constexpr auto operator+(const Strict<Num>& a) -> Strict<Num> {
  return Strict{+a.get()};
}

/// Number addition.
template<class Num>
constexpr auto operator+(const Strict<Num>& a,
                         const Strict<Num>& b) -> Strict<Num> {
  return Strict{a.get() + b.get()};
}

/// Number addition with assignment.
template<class Num>
constexpr auto operator+=(Strict<Num>& a,
                          const Strict<Num>& b) -> Strict<Num>& {
  a.get() += b.get();
  return a;
}

/// Number negation.
template<class Num>
constexpr auto operator-(const Strict<Num>& a) -> Strict<Num> {
  return Strict{-a.get()};
}

/// Number subtraction.
template<class Num>
constexpr auto operator-(const Strict<Num>& a,
                         const Strict<Num>& b) -> Strict<Num> {
  return Strict{a.get() - b.get()};
}

/// Number subtraction with assignment.
template<class Num>
constexpr auto operator-=(Strict<Num>& a,
                          const Strict<Num>& b) -> Strict<Num>& {
  a.get() -= b.get();
  return a;
}

/// Number multiplication.
template<class Num>
constexpr auto operator*(const Strict<Num>& a,
                         const Strict<Num>& b) -> Strict<Num> {
  return Strict{a.get() * b.get()};
}

/// Number multiplication with assignment.
template<class Num>
constexpr auto operator*=(Strict<Num>& a,
                          const Strict<Num>& b) -> Strict<Num>& {
  a.get() *= b.get();
  return a;
}

/// Number division.
template<class Num>
constexpr auto operator/(const Strict<Num>& a,
                         const Strict<Num>& b) -> Strict<Num> {
  return Strict{a.get() / b.get()};
}

/// Number division with assignment.
template<class Num>
constexpr auto operator/=(Strict<Num>& a,
                          const Strict<Num>& b) -> Strict<Num>& {
  a.get() /= b.get();
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number absolute value.
template<class Num>
constexpr auto abs(const Strict<Num>& a) -> Strict<Num> {
  return Strict{abs(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the largest integer value not greater than the number.
template<class Num>
constexpr auto floor(const Strict<Num>& a) -> Strict<Num> {
  return Strict{floor(a.get())};
}

/// Compute the nearest integer value to the number.
template<class Num>
constexpr auto round(const Strict<Num>& a) -> Strict<Num> {
  return Strict{round(a.get())};
}

/// Compute the smallest integer value not less than the number.
template<class Num>
constexpr auto ceil(const Strict<Num>& a) -> Strict<Num> {
  return Strict{ceil(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Raise the number to the power.
/// @{
template<class Num>
constexpr auto pow(const Strict<Num>& a, const Strict<Num>& b) -> Strict<Num> {
  return Strict{pow(a.get(), b.get())};
}
template<class Num>
constexpr auto pow(const Strict<Num>& a,
                   const std::type_identity_t<Num>& b) -> Strict<Num> {
  return Strict{pow(a.get(), b)};
}
/// @}

/// Compute the square root of the number.
template<class Num>
constexpr auto sqrt(const Strict<Num>& a) -> Strict<Num> {
  return Strict{sqrt(a.get())};
}

/// Compute the reciprocal square root of the number.
template<class Num>
constexpr auto rsqrt(const Strict<Num>& a) -> Strict<Num> {
  return Strict{rsqrt(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
