/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "tit/core/math.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dual number.
template<class Num, class Deriv = Num>
class Dual final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a dual number.
  constexpr explicit Dual(Num val = {}, Deriv deriv = {}) noexcept
      : val_{std::move(val)}, deriv_{std::move(deriv)} {}

  /// Get the value part.
  constexpr auto val() const noexcept -> const Num& {
    return val_;
  }

  /// Get the derivative part.
  constexpr auto deriv() const noexcept -> const Deriv& {
    return deriv_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Dual number unary plus operator.
  friend constexpr auto operator+(const Dual& f) -> Dual {
    return f;
  }

  /// Dual number addition.
  friend constexpr auto operator+(const Dual& f, const Dual& g) -> Dual {
    return Dual{f.val() + g.val(), f.deriv() + g.deriv()};
  }

  /// Dual number addition with assignment.
  friend constexpr auto operator+=(Dual& f, const Dual& g) -> Dual& {
    return f = f + g;
  }

  /// Dual number negation.
  friend constexpr auto operator-(const Dual& f) -> Dual {
    return Dual{-f.val(), -f.deriv()};
  }

  /// Dual number subtraction.
  friend constexpr auto operator-(const Dual& f, const Dual& g) -> Dual {
    return Dual{f.val() - g.val(), f.deriv() - g.deriv()};
  }

  /// Dual number subtraction with assignment.
  friend constexpr auto operator-=(Dual& f, const Dual& g) -> Dual& {
    return f = f - g;
  }

  /// Dual number multiplication.
  /// @{
  friend constexpr auto operator*(const Num& a, const Dual& f) -> Dual {
    return Dual{a * f.val(), a * f.deriv()};
  }
  friend constexpr auto operator*(const Dual& f, const Num& a) -> Dual {
    return Dual{f.val() * a, f.deriv() * a};
  }
  friend constexpr auto operator*(const Dual& f, const Dual& g) -> Dual {
    return Dual{f.val() * g.val(), f.deriv() * g.val() + f.val() * g.deriv()};
  }
  /// @}

  /// Dual number multiplication with assignment.
  /// @{
  friend constexpr auto operator*=(Dual& f, const Num& a) -> Dual& {
    f = f * a;
    return f;
  }
  friend constexpr auto operator*=(Dual& f, const Dual& g) -> Dual& {
    f = f * g;
    return f;
  }
  /// @}

  /// Dual number division.
  /// @{
  friend constexpr auto operator/(const Dual& f, const Num& a) -> Dual {
    return Dual{f.val() / a, f.deriv() / a};
  }
  friend constexpr auto operator/(const Num& a, const Dual& f) -> Dual {
    return Dual{a / f.val(), -a * f.deriv() / pow2(f.val())};
  }
  friend constexpr auto operator/(const Dual& f, const Dual& g) -> Dual {
    return Dual{f.val() / g.val(),
                (f.deriv() * g.val() - f.val() * g.deriv()) / pow2(g.val())};
  }
  /// @}

  /// Dual number division with assignment.
  /// @{
  friend constexpr auto operator/=(Dual& f, const Num& a) -> Dual& {
    f = f / a;
    return f;
  }
  friend constexpr auto operator/=(Dual& f, const Dual& g) -> Dual& {
    f = f / g;
    return f;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare two numbers by value.
  /// @{
  friend constexpr auto operator==(const Dual& f, const Dual& g) noexcept {
    return f.val_ == g.val_;
  }
  friend constexpr auto operator<=>(const Dual& f, const Dual& g) noexcept {
    return f.val_ <=> g.val_;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Num val_;
  Deriv deriv_;

}; // class Dual

template<std::floating_point Float, class Deriv>
constexpr auto tiny_v<Dual<Float, Deriv>> = Dual{tiny_v<Float>, Deriv{}};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Square root of a dual number.
template<class Num, class Deriv>
constexpr auto sqrt(const Dual<Num, Deriv>& f) -> Dual<Num, Deriv> {
  return Dual{sqrt(f.val()), f.deriv() / (Num{2.0} * sqrt(f.val()))};
}

/// Raise a dual number to a power.
/// @{
template<class Num, class Deriv>
constexpr auto pow(const Dual<Num, Deriv>& f, std::type_identity_t<Num> a)
    -> Dual<Num, Deriv> {
  return Dual{pow(f.val(), a), a * pow(f.val(), a - Num{1.0}) * f.deriv()};
}
template<class Num, class Deriv>
constexpr auto pow(const Dual<Num, Deriv>& f, const Dual<Num, Deriv>& g)
    -> Dual<Num, Deriv> {
  return Dual{pow(f.val(), g.val()),
              pow(f.val(), g.val() - Num{1.0}) *
                  (g.val() * f.deriv() + f.val() * log(f.val()) * g.deriv())};
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exponential of a dual number.
template<class Num, class Deriv>
constexpr auto exp(const Dual<Num, Deriv>& f) -> Dual<Num, Deriv> {
  return Dual{exp(f.val()), exp(f.val()) * f.deriv()};
}

/// Natural logarithm of a dual number.
template<class Num, class Deriv>
constexpr auto log(const Dual<Num, Deriv>& f) -> Dual<Num, Deriv> {
  return Dual{log(f.val()), f.deriv() / f.val()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
