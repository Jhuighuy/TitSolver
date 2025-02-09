/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <utility>

#include "tit/core/math.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrapper for the numerical type. Use is to prevent explicit specializations
/// for the built-in numerical types.
///
/// @tparam Num Underlying numerical type.
/// @tparam Tag Tag type. Strict numbers with different tags considered
///             different types, operations between such numbers are not
///             defined.
template<class Num, class Tag = void>
class Strict final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a number.
  constexpr Strict() noexcept = default;

  /// Initialize a number with a built-in numerical value.
  constexpr explicit Strict(Num val) : val_{val} {}

  /// Get the underlying value.
  constexpr auto get(this auto&& self) noexcept -> auto&& {
    return std::forward<decltype(self)>(self).val_;
  }

  /// Cast number to a different type.
  template<class To>
    requires std::convertible_to<Num, To>
  constexpr explicit operator To() const noexcept {
    return static_cast<To>(val_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number unary plus operator.
  friend constexpr auto operator+(const Strict& a) -> Strict {
    return Strict{+a.get()};
  }

  /// Number addition.
  friend constexpr auto operator+(const Strict& a, const Strict& b) -> Strict {
    return Strict{a.get() + b.get()};
  }

  /// Number addition with assignment.
  friend constexpr auto operator+=(Strict& a, const Strict& b) -> Strict& {
    a.get() += b.get();
    return a;
  }

  /// Number negation.
  friend constexpr auto operator-(const Strict& a) -> Strict {
    return Strict{-a.get()};
  }

  /// Number subtraction.
  friend constexpr auto operator-(const Strict& a, const Strict& b) -> Strict {
    return Strict{a.get() - b.get()};
  }

  /// Number subtraction with assignment.
  friend constexpr auto operator-=(Strict& a, const Strict& b) -> Strict& {
    a.get() -= b.get();
    return a;
  }

  /// Number multiplication.
  friend constexpr auto operator*(const Strict& a, const Strict& b) -> Strict {
    return Strict{a.get() * b.get()};
  }

  /// Number multiplication with assignment.
  friend constexpr auto operator*=(Strict& a, const Strict& b) -> Strict& {
    a.get() *= b.get();
    return a;
  }

  /// Number division.
  friend constexpr auto operator/(const Strict& a, const Strict& b) -> Strict {
    return Strict{a.get() / b.get()};
  }

  /// Number division with assignment.
  friend constexpr auto operator/=(Strict& a, const Strict& b) -> Strict& {
    a.get() /= b.get();
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare two numbers by value.
  constexpr auto operator<=>(const Strict&) const noexcept = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Num val_;

}; // class Strict

template<std::floating_point Float>
constexpr auto tiny_v<Strict<Float>> = Strict{tiny_v<Float>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number absolute value.
template<class Num>
constexpr auto abs(const Strict<Num>& a) -> Strict<Num> {
  return Strict{abs(a.get())};
}

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

/// Compute the square root of the number.
template<class Num>
constexpr auto sqrt(const Strict<Num>& a) -> Strict<Num> {
  return Strict{sqrt(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
