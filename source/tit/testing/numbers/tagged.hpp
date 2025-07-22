/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrapper for the numerical type. Use is to prevent explicit specializations
/// for the built-in numerical types.
///
/// @tparam Num Underlying numerical type.
/// @tparam Tag Tag type. Tagged numbers with different tags considered
///             different types, operations between such numbers are not
///             defined.
template<class Num, class Tag = void>
class Tagged final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a number.
  constexpr Tagged() noexcept = default;

  /// Initialize a number with a built-in numerical value.
  constexpr explicit Tagged(Num val) : val_{val} {}

  /// Get the underlying value.
  constexpr auto get(this auto&& self) noexcept -> auto&& {
    return TIT_FORWARD_LIKE(self, self.val_);
  }

  /// Cast number to a different type.
  template<class To>
    requires std::convertible_to<Num, To>
  constexpr explicit operator To() const noexcept {
    return static_cast<To>(val_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number unary plus operator.
  friend constexpr auto operator+(const Tagged& a) -> Tagged {
    return Tagged{+a.get()};
  }

  /// Number addition.
  friend constexpr auto operator+(const Tagged& a, const Tagged& b) -> Tagged {
    return Tagged{a.get() + b.get()};
  }

  /// Number addition with assignment.
  friend constexpr auto operator+=(Tagged& a, const Tagged& b) -> Tagged& {
    a.get() += b.get();
    return a;
  }

  /// Number negation.
  friend constexpr auto operator-(const Tagged& a) -> Tagged {
    return Tagged{-a.get()};
  }

  /// Number subtraction.
  friend constexpr auto operator-(const Tagged& a, const Tagged& b) -> Tagged {
    return Tagged{a.get() - b.get()};
  }

  /// Number subtraction with assignment.
  friend constexpr auto operator-=(Tagged& a, const Tagged& b) -> Tagged& {
    a.get() -= b.get();
    return a;
  }

  /// Number multiplication.
  friend constexpr auto operator*(const Tagged& a, const Tagged& b) -> Tagged {
    return Tagged{a.get() * b.get()};
  }

  /// Number multiplication with assignment.
  friend constexpr auto operator*=(Tagged& a, const Tagged& b) -> Tagged& {
    a.get() *= b.get();
    return a;
  }

  /// Number division.
  friend constexpr auto operator/(const Tagged& a, const Tagged& b) -> Tagged {
    return Tagged{a.get() / b.get()};
  }

  /// Number division with assignment.
  friend constexpr auto operator/=(Tagged& a, const Tagged& b) -> Tagged& {
    a.get() /= b.get();
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare two numbers by value.
  constexpr auto operator<=>(const Tagged&) const noexcept = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Num val_;

}; // class Tagged

template<std::floating_point Float>
constexpr auto tiny_v<Tagged<Float>> = Tagged{tiny_v<Float>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Number absolute value.
template<class Num>
constexpr auto abs(const Tagged<Num>& a) -> Tagged<Num> {
  return Tagged{abs(a.get())};
}

/// Compute the largest integer value not greater than the number.
template<class Num>
constexpr auto floor(const Tagged<Num>& a) -> Tagged<Num> {
  return Tagged{floor(a.get())};
}

/// Compute the nearest integer value to the number.
template<class Num>
constexpr auto round(const Tagged<Num>& a) -> Tagged<Num> {
  return Tagged{round(a.get())};
}

/// Compute the smallest integer value not less than the number.
template<class Num>
constexpr auto ceil(const Tagged<Num>& a) -> Tagged<Num> {
  return Tagged{ceil(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the square root of the number.
template<class Num>
constexpr auto sqrt(const Tagged<Num>& a) -> Tagged<Num> {
  return Tagged{sqrt(a.get())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
