/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <cmath>
#include <limits>

#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"
#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Negation result type. */
template<class Num>
using negate_result_t = decltype(-std::declval<Num>());

/** Addition result type. */
template<class NumA, class NumB = NumA>
using add_result_t = decltype(std::declval<NumA>() + std::declval<NumB>());

/** Subtraction result type. */
template<class NumA, class NumB = NumA>
using sub_result_t = decltype(std::declval<NumA>() - std::declval<NumB>());

/** Multiplication result type. */
template<class NumA, class NumB = NumA>
using mul_result_t = decltype(std::declval<NumA>() * std::declval<NumB>());

/** Division result type. */
template<class NumA, class NumB = NumA>
using div_result_t = decltype(std::declval<NumA>() / std::declval<NumB>());

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Absolute value. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto abs(Num a) noexcept -> Num {
  return std::abs(a);
}

/** Sign of the value. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto sign(Num a) noexcept -> Num {
  return Num{Num{0} < a} - Num{a < Num{0}};
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Small number, treated as zero. */
template<std::floating_point Real>
constexpr auto small_number_v {
#if TIT_LIBCPP // libc++'s `cbrt` is not constexpr yet.
  std::numeric_limits<Real>::epsilon()
#else
  std::cbrt(std::numeric_limits<Real>::epsilon())
#endif
};

/** Check if number is approximately zero. */
template<std::floating_point Real>
constexpr auto is_zero(Real a) noexcept -> bool {
  return abs(a) < small_number_v<Real>;
}
/** Check if numbers are approximately equal. */
template<std::floating_point Real>
constexpr auto approx_equal(Real a, Real b) noexcept -> bool {
  return is_zero(a - b);
}

/** Positive a or zero. */
template<class Num>
constexpr auto plus(Num a) noexcept -> Num {
  return std::max(Num{0}, a);
}
/** Negative a or zero. */
template<class Num>
constexpr auto minus(Num a) noexcept -> Num {
  return std::min(Num{0}, a);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute the largest integer value not greater than number. */
template<std::floating_point Real>
constexpr auto floor(Real a) noexcept -> Real {
  return std::floor(a);
}

/** Computes the nearest integer value to number. */
template<std::floating_point Real>
constexpr auto round(Real a) noexcept -> Real {
  return std::round(a);
}

/** Compute the least integer value not less than number. */
template<std::floating_point Real>
constexpr auto ceil(Real a) noexcept -> Real {
  return std::ceil(a);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Inverse number. */
/** @{ */
template<std::floating_point Real>
constexpr auto inverse(Real a) noexcept -> Real {
  TIT_ASSERT(!is_zero(a), "Cannot invert zero!");
  return Real{1.0} / a;
}
template<std::integral Int>
constexpr auto inverse(Int a) noexcept -> real_t {
  TIT_ASSERT(a != 0, "Cannot invert zero!");
  return 1.0 / a;
}
/** @} */

/** Safe inverse number.
 ** @returns Inverse for non-zero input, zero for zero input. */
template<std::floating_point Real>
constexpr auto safe_inverse(Real a) noexcept -> Real {
  return is_zero(a) ? Real{0.0} : inverse(a);
}
/** Safe divide number by divisor.
 ** @returns Division result for non-zero divisor, zero for zero divisor. */
template<class Num, std::floating_point Real>
constexpr auto safe_divide(Num a, Real b) noexcept {
  return is_zero(b) ? div_result_t<Num, Real>{} : a / b;
}

/** Ceiling divide unsigned integer. */
template<std::unsigned_integral UInt>
constexpr auto ceil_divide(UInt a, UInt b) noexcept -> UInt {
  return (a + b - UInt{1}) / b;
}
/** Align unsigned integer. */
template<std::unsigned_integral UInt>
constexpr auto align(UInt a, UInt alignment) noexcept -> UInt {
  return ceil_divide(a, alignment) * alignment;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Raise to the second power with 1 multiplication. */
template<class Num>
constexpr auto pow2(Num a) noexcept -> Num {
  return a * a;
}
/** Raise to the third power with 2 multiplications. */
template<class Num>
constexpr auto pow3(Num a) noexcept -> Num {
  return a * a * a;
}
/** Raise to the fourth power with 2 multiplications. */
template<class Num>
constexpr auto pow4(Num a) noexcept -> Num {
  const auto a_sqr = a * a;
  return a_sqr * a_sqr;
}
/** Raise to the fifth power with 3 multiplications. */
template<class Num>
constexpr auto pow5(Num a) noexcept -> Num {
  const auto a_sqr = a * a;
  return a_sqr * a_sqr * a;
}
/** Raise to the sixth power with 3 multiplications. */
template<class Num>
constexpr auto pow6(Num a) noexcept -> Num {
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed;
}
/** Raise to the seventh power with 4 multiplications. */
template<class Num>
constexpr auto pow7(Num a) noexcept -> Num {
  // TODO: can `pow7` be implemented with 3 multiplications?
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed * a;
}
/** Raise to the eighth power with 3 multiplications. */
template<class Num>
constexpr auto pow8(Num a) noexcept -> Num {
  const auto a_sqr = a * a;
  const auto a_pow4 = a_sqr * a_sqr;
  return a_pow4 * a_pow4;
}
/** Raise to the nineth power with 4 multiplications. */
template<class Num>
constexpr auto pow9(Num a) noexcept -> Num {
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed * a_cubed;
}
/** Raise to the power. */
template<class Num>
constexpr auto pow(Num a, std::type_identity_t<Num> power) noexcept -> Num {
  return std::pow(a, power);
}

/** Square root. */
template<std::floating_point Real>
constexpr auto sqrt(Real a) noexcept -> Real {
  return std::sqrt(a);
}

/** Cube root. */
template<std::floating_point Real>
constexpr auto cbrt(Real a) noexcept -> Real {
  return std::cbrt(a);
}

/** Hypot. */
/** @{ */
template<std::floating_point Real>
constexpr auto hypot(Real a, Real b) noexcept -> Real {
  return std::hypot(a, b);
}
template<std::floating_point Real>
constexpr auto hypot(Real a, Real b, Real c) noexcept -> Real {
  return std::hypot(a, b, c);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Exponent. */
template<std::floating_point Real>
constexpr auto exp(Real a) noexcept -> Real {
  return std::exp(a);
}
/** Exponent base two. */
/** @{ */
template<std::floating_point Real>
constexpr auto exp2(Real a) noexcept -> Real {
  return std::exp2(a);
}
template<std::unsigned_integral UInt>
constexpr auto exp2(UInt a) noexcept -> UInt {
  return UInt{1} << a;
}
/** @} */

/** Logarithm. */
template<std::floating_point Real>
constexpr auto log(Real a) noexcept -> Real {
  return std::log(a);
}

/** Logarithm base two. */
/** @{ */
template<std::floating_point Real>
constexpr auto log2(Real a) noexcept -> Real {
  return std::log2(a);
}
template<std::unsigned_integral UInt>
constexpr auto log2(UInt a) noexcept -> UInt {
  return std::bit_width(a) - UInt{1};
}
/** @} */

/** Check if integer a is power of two. */
template<std::unsigned_integral UInt>
constexpr auto is_power_of_two(UInt a) noexcept -> bool {
  return (a & (a - UInt{1})) == UInt{0};
}

/** Align-up integer to the nearest power of two. */
template<std::unsigned_integral UInt>
constexpr auto align_to_power_of_two(UInt a) noexcept -> UInt {
  // TODO: maybe a branchless implemention?
  return is_power_of_two(a) ? a : exp2(log2(a) + 1);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Average function. */
template<class... Nums>
constexpr auto avg(Nums... values) noexcept {
  return (values + ...) / (sizeof...(Nums));
}
/** Harmonic average function. */
template<class... Nums>
constexpr auto havg(Nums... values) noexcept {
  return (sizeof...(Nums)) / (inverse(values) + ...);
}
/** Geometric average functions. */
template<class... Nums>
constexpr auto gavg(Nums... values) noexcept {
  return pow((values * ...), inverse(sizeof...(Nums)));
}

/** Merge number with zero vector based on condition. */
template<class Num>
constexpr auto merge(bool m, Num a) noexcept {
  // Supposed to be overriden by intrisics or optimized-out.
  return m * a;
}
/** Merge two numbers with zero vector based on condition. */
template<class NumA, class NumB>
constexpr auto merge(bool m, NumA a, NumB b) noexcept {
  // Supposed to be overriden by intrisics or optimized-out.
  return m * a + (!m) * b;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Find function rool using Newton-Raphson method.
 ** @param x Current estimate of the root.
 ** @param f Function whose root we are looking for.
 **          Should return a pair of it's value and derivative.
 **          Should implicitly depend on @p x
 ** @param eps Tolerance.
 ** @param max_iter Maximum amount of iterations.
 ** @returns True on success. */
template<std::floating_point Real, std::invocable Func>
constexpr auto newton_raphson(Real& x, const Func& f, //
                              Real eps = Real{1.0e-9}, size_t max_iter = 10)
    -> bool {
  for (size_t iter = 0; iter < max_iter; ++iter) {
    const auto [y, df_dx] = std::invoke(f /*, x*/);
    if (abs(y) <= eps) return true;
    if (is_zero(df_dx)) break;
    x -= y / df_dx;
  }
  return false;
}

/** Find function rool using Bisection method.
 ** @param min_x Root's lower bound.
 ** @param max_x Root's upper bound.
 ** @param f Function whose root we are looking for.
 ** @param eps Tolerance.
 ** @param max_iter Maximum amount of iterations.
 ** @returns True on success. */
template<std::floating_point Real, std::invocable<Real> Func>
constexpr auto bisection(Real& min_x, Real& max_x, const Func& f,
                         Real eps = Real{1.0e-9}, size_t max_iter = 10)
    -> bool {
  TIT_ASSERT(min_x <= max_x, "Inverted search range!");
  auto min_f = std::invoke(f, min_x);
  // Check for the region bounds first.
  if (abs(min_f) <= eps) {
    max_x = min_x;
    return true;
  }
  auto max_f = std::invoke(f, max_x);
  if (abs(max_f) <= eps) {
    min_x = max_x;
    return true;
  }
  if (sign(max_f) == sign(min_f)) return false;
  for (size_t iter = 0; iter < max_iter; ++iter) {
    // Approximate f(x) with line equation:
    // f(x) = min_f + (max_f - min_f)/(max_x - min_x) * (x - min_x),
    // so approximate root of f(x) == 0 is:
    const Real x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    const auto y = std::invoke(f, x);
    if (abs(y) <= eps) {
      min_x = max_x = x;
      return true;
    }
    if (sign(min_f) == sign(y)) {
      min_x = x, min_f = y;
    } else if (sign(max_f) == sing(y)) {
      max_x = x, max_f = y;
    }
  }
  return false;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit