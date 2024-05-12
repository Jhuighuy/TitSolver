/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/math.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrapper for the numerical type. Use is to prevent explicit specializations
/// for the built-in numerical types.
template<class Num>
class Strict final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a number.
  constexpr Strict() noexcept = default;

  /// Initialize a number with a built-in numerical value.
  constexpr explicit Strict(Num val) : val_{val} {}

  /// Get the underlying value.
  /// @{
  constexpr auto get() noexcept -> Num& {
    return val_;
  }
  constexpr auto get() const noexcept -> Num const& {
    return val_;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Cast number to a different type.
  template<class To>
    requires std::convertible_to<Num, To>
  constexpr explicit operator To() const noexcept {
    return static_cast<To>(val_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number unary plus operator.
  friend constexpr auto operator+(Strict const& a) -> Strict {
    return Strict{+a.get()};
  }

  /// Number addition.
  friend constexpr auto operator+(Strict const& a, Strict const& b) -> Strict {
    return Strict{a.get() + b.get()};
  }

  /// Number addition with assignment.
  friend constexpr auto operator+=(Strict& a, Strict const& b) -> Strict& {
    a.get() += b.get();
    return a;
  }

  /// Number negation.
  friend constexpr auto operator-(Strict const& a) -> Strict {
    return Strict{-a.get()};
  }

  /// Number subtraction.
  friend constexpr auto operator-(Strict const& a, Strict const& b) -> Strict {
    return Strict{a.get() - b.get()};
  }

  /// Number subtraction with assignment.
  friend constexpr auto operator-=(Strict& a, Strict const& b) -> Strict& {
    a.get() -= b.get();
    return a;
  }

  /// Number multiplication.
  friend constexpr auto operator*(Strict const& a, Strict const& b) -> Strict {
    return Strict{a.get() * b.get()};
  }

  /// Number multiplication with assignment.
  friend constexpr auto operator*=(Strict& a, Strict const& b) -> Strict& {
    a.get() *= b.get();
    return a;
  }

  /// Number division.
  friend constexpr auto operator/(Strict const& a, Strict const& b) -> Strict {
    return Strict{a.get() / b.get()};
  }

  /// Number division with assignment.
  friend constexpr auto operator/=(Strict& a, Strict const& b) -> Strict& {
    a.get() /= b.get();
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare two numbers by value.
  constexpr auto operator<=>(Strict const&) const noexcept = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Num val_;

}; // class Strict

template<std::floating_point Real>
constexpr auto tiny_number_v<Strict<Real>> = Strict{tiny_number_v<Real>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number absolute value.
template<class Num>
constexpr auto abs(Strict<Num> const& a) -> Strict<Num> {
  return Strict{abs(a.get())};
}

/// Compute the largest integer value not greater than the number.
template<class Num>
constexpr auto floor(Strict<Num> const& a) -> Strict<Num> {
  return Strict{floor(a.get())};
}

/// Compute the nearest integer value to the number.
template<class Num>
constexpr auto round(Strict<Num> const& a) -> Strict<Num> {
  return Strict{round(a.get())};
}

/// Compute the smallest integer value not less than the number.
template<class Num>
constexpr auto ceil(Strict<Num> const& a) -> Strict<Num> {
  return Strict{ceil(a.get())};
}

/// Compute the square root of the number.
template<class Num>
constexpr auto sqrt(Strict<Num> const& a) -> Strict<Num> {
  return Strict{sqrt(a.get())};
}

/// Compute the reciprocal square root of the number.
template<class Num>
constexpr auto rsqrt(Strict<Num> const& a) -> Strict<Num> {
  return Strict{rsqrt(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
