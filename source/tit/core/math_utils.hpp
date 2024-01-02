/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cmath>
#include <limits>

#include <algorithm>
#include <concepts> // IWYU pragma: keep
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <type_traits>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/misc.hpp"
#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Negation result type. */
template<class Num>
using negate_result_t = decltype(-std::declval<Num>());

/** Addition result type. */
template<class NumA, class NumB>
using add_result_t = decltype(std::declval<NumA>() + std::declval<NumB>());

/** Subtraction result type. */
template<class NumA, class NumB>
using sub_result_t = decltype(std::declval<NumA>() - std::declval<NumB>());

/** Multiplication result type. */
template<class NumA, class NumB>
using mul_result_t = decltype(std::declval<NumA>() * std::declval<NumB>());

/** Division result type. */
template<class NumA, class NumB>
using div_result_t = decltype(std::declval<NumA>() / std::declval<NumB>());

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Absolute value. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto abs(Num a) noexcept -> Num {
  return std::abs(a);
}

/** Positive a or zero. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto plus(Num a) noexcept -> Num {
  // DO NOT CHANGE ORDER OF THE ARGUMENTS!
  // This order of the arguments ensures correct NaN propagation.
  return std::max(a, Num{0});
}
/** Negative a or zero. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto minus(Num a) noexcept -> Num {
  // DO NOT CHANGE ORDER OF THE ARGUMENTS!
  // This order of the arguments ensures correct NaN propagation.
  return std::min(a, Num{0});
}

/** Sign of the value. */
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto sign(Num a) noexcept -> int {
  return int{Num{0} < a} - int{a < Num{0}};
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Small number, treated as zero. */
template<std::floating_point Real>
constexpr auto small_number_v =
#ifdef __clang__ // clang's `cbrt` is not constexpr yet.
    std::numeric_limits<Real>::epsilon()
#else
    std::cbrt(std::numeric_limits<Real>::epsilon())
#endif
    ;

/** Check if number is approximately zero. */
template<std::floating_point Real>
constexpr auto is_zero(Real a) noexcept -> bool {
  return abs(a) <= small_number_v<Real>;
}

/** Check if two numbers are approximately equal. */
template<std::floating_point Real>
constexpr auto approx_eq(Real a, Real b) noexcept -> bool {
  return is_zero(a - b);
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

/** Inverse number.
 ** @returns Floating-point reciprocal. */
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
/** Divide numbers.
 ** @returns Floating-point division result. */
/** @{ */
template<class Num, std::floating_point Real>
constexpr auto divide(Num a, Real b) noexcept -> Real {
  TIT_ASSERT(!is_zero(b), "Cannot divide by zero!");
  return a / b;
}
template<class Num, std::integral Int>
constexpr auto divide(Num a, Int b) noexcept {
  TIT_ASSERT(b != 0, "Cannot divide by zero!");
  return a / static_cast<real_t>(b);
}
/** @} */

/** Safe inverse number.
 ** @returns Inverse for non-zero input, zero for zero input. */
template<std::floating_point Real>
constexpr auto safe_inverse(Real a) noexcept -> Real {
  return is_zero(a) ? Real{0.0} : inverse(a);
}
/** Safe divide number by divisor.
 ** @returns Division result for non-zero divisor, zero if divisor is small. */
template<class Num, std::floating_point Real>
constexpr auto safe_divide(Num a, Real b) noexcept {
  return is_zero(b) ? div_result_t<Num, Real>{} : a / b;
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
/** Raise to the ninth power with 4 multiplications. */
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

/** Evaluate polynomial @f$ \sum c_k x^k @f$ value. */
template<class Num, class Coeff>
constexpr auto horner(Num x, std::initializer_list<Coeff> ci) noexcept {
  mul_result_t<Num, Coeff> r{0};
  for (const auto c : ci | std::views::reverse) r = r * x + c;
  return r;
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

/** Arithmetic average function.
 ** @note Presence of infinities of different signs will generate NaN. */
template<class... Nums>
  requires (sizeof...(Nums) > 0)
constexpr auto avg(Nums... values) noexcept {
  return divide((values + ...), sizeof...(Nums));
}
/** Harmonic average function.
 ** @param values Input values. Must be positive. */
template<std::floating_point... Reals>
  requires (sizeof...(Reals) > 0)
constexpr auto havg(Reals... values) noexcept {
  TIT_ASSERT(((values >= 0) && ...),
             "Harmonic average requires all non-negative input.");
  return sizeof...(Reals) / (inverse(values) + ...);
}
/** Geometric average functions.
 ** @param values Input values. Must be positive. */
template<std::floating_point... Reals>
  requires (sizeof...(Reals) > 0)
constexpr auto gavg(Reals... values) noexcept {
  TIT_ASSERT(((values >= 0) && ...),
             "Geometric average requires all non-negative input.");
  return pow((values * ...), inverse(sizeof...(Reals)));
}

/** @brief Merge number with zero vector based on condition.
 ** Implementation of this function is supposed to be branchless.
 **
 ** @note This function does not propagate floating-point infinites or NaNs
 **       from @p a condition is `false`. */
template<std::floating_point Real>
constexpr auto merge(bool m, Real a) noexcept -> Real {
  // Supposed to be overridden by intrinsics or optimized-out.
  // TODO: implement with bit operations.
  return m ? a : Real{0.0};
}
/** @brief Select one of two numbers vector based on condition.
 ** Implementation of this function is supposed to be branchless.
 **
 ** @note This function does not propagate floating-point infinites or NaNs
 **       from the argument which was not selected. */
template<std::floating_point Real>
constexpr auto merge(bool m, Real a, Real b) noexcept -> Real {
  // Supposed to be overridden by intrinsics or optimized-out.
  // TODO: implement with bit operations.
  return m ? a : b;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Newton-Raphson solver status. */
enum class NewtonRaphsonStatus {
  /** Success. */
  success,
  /** Failure: number of iterations exceeded. */
  failure_max_iter,
  /** Failure: computation reached a zero derivative. */
  failure_zero_deriv,
}; // enum class NewtonRaphsonStatus

/** Find function rool using Newton-Raphson method.
 ** @param x Current estimate of the root.
 ** @param f Function whose root we are looking for.
 **          Should return a pair of it's value and derivative.
 **          Should implicitly depend on @p x.
 ** @param eps Tolerance.
 ** @param max_iter Maximum amount of iterations.
 ** @returns True on success. */
template<std::floating_point Real, class Func>
  requires std::invocable<Func&&>
constexpr auto newton_raphson(Real& x, Func&& f,
                              Real eps = small_number_v<Real>,
                              size_t max_iter = 10) -> NewtonRaphsonStatus {
  TIT_ASSUME_UNIVERSAL(Func, f);
  using enum NewtonRaphsonStatus;
  for (size_t iter = 0; iter < max_iter; ++iter) {
    const auto [y, df_dx] = std::invoke(f /*, x*/);
    if (abs(y) <= eps) return success;
    if (is_zero(df_dx)) return failure_zero_deriv;
    x -= y / df_dx;
  }
  return failure_max_iter;
}

/** Bisection solver status. */
enum class BisectionStatus {
  /** Success. */
  success,
  /** Failure: number of iterations exceeded. */
  failure_max_iter,
  /** Failure: function has same signs on the ends of the search range. */
  failure_sign,
}; // enum class BisectionStatus

/** Find function rool using Bisection method.
 ** @param min_x Root's lower bound.
 ** @param max_x Root's upper bound.
 ** @param f Function whose root we are looking for.
 ** @param eps Tolerance.
 ** @param max_iter Maximum amount of iterations.
 ** @returns True on success. */
template<std::floating_point Real, class Func>
  requires std::invocable<Func&&, Real> &&
           std::assignable_from<Real&, std::invoke_result_t<Func&&, Real>>
constexpr auto bisection(Real& min_x, Real& max_x, Func&& f,
                         Real eps = small_number_v<Real>, //
                         size_t max_iter = 10) -> BisectionStatus {
  TIT_ASSUME_UNIVERSAL(Func, f);
  TIT_ASSERT(min_x <= max_x, "Inverted search range!");
  using enum BisectionStatus;
  // Check for the region bounds first.
  auto min_f = std::invoke(f, min_x);
  if (abs(min_f) <= eps) {
    max_x = min_x;
    return success;
  }
  auto max_f = std::invoke(f, max_x);
  if (abs(max_f) <= eps) {
    min_x = max_x;
    return success;
  }
  for (size_t iter = 0; iter < max_iter; ++iter) {
    if (sign(max_f) == sign(min_f)) return failure_sign;
    // Approximate f(x) with line equation:
    // f(x) = min_f + (max_f - min_f)/(max_x - min_x) * (x - min_x),
    // so approximate root of f(x) == 0 is:
    const auto x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    const auto y = std::invoke(f, x);
    if (abs(y) <= eps) {
      min_x = max_x = x;
      return success;
    }
    const auto sign_y = sign(y);
    if (sign_y != sign(min_f)) max_x = x, max_f = y;
    else if (sign_y != sign(max_f)) min_x = x, min_f = y;
  }
  return failure_max_iter;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
