/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Negation result type.
template<class Num>
using negate_result_t = decltype(-std::declval<Num>());

/// Addition result type.
template<class NumA, class NumB = NumA>
using add_result_t = decltype(std::declval<NumA>() + std::declval<NumB>());

/// Subtraction result type.
template<class NumA, class NumB = NumA>
using sub_result_t = decltype(std::declval<NumA>() - std::declval<NumB>());

/// Multiplication result type.
template<class NumA, class NumB = NumA>
using mul_result_t = decltype(std::declval<NumA>() * std::declval<NumB>());

/// Division result type.
template<class NumA, class NumB = NumA>
using div_result_t = decltype(std::declval<NumA>() / std::declval<NumB>());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Absolute value.
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto abs(Num a) noexcept -> Num {
  return std::abs(a);
}

/// Positive a or zero.
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto plus(Num a) noexcept -> Num {
  // DO NOT CHANGE ORDER OF THE ARGUMENTS!
  // This order of the arguments ensures correct NaN propagation.
  return std::max(a, Num{0});
}
/// Negative a or zero.
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto minus(Num a) noexcept -> Num {
  // DO NOT CHANGE ORDER OF THE ARGUMENTS!
  // This order of the arguments ensures correct NaN propagation.
  return std::min(a, Num{0});
}

/// Sign of the value.
template<class Num>
  requires (!std::unsigned_integral<Num>)
constexpr auto sign(Num a) noexcept -> Num {
  auto const s = static_cast<Num>(Num{0} < a) - static_cast<Num>(a < Num{0});
  if constexpr (std::integral<Num>) return s;
  else {
    // We need to do this extra division in order to propagate NaN correctly.
    // Maybe there is a better way.
    return s / (Num{1} - static_cast<Num>(std::isnan(a)));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Small number, treated as zero.
template<std::floating_point Real>
constexpr auto small_number_v =
#ifdef __clang__ // clang's `cbrt` is not constexpr yet.
    std::numeric_limits<Real>::epsilon()
#else
    std::cbrt(std::numeric_limits<Real>::epsilon())
#endif
    ;

/// Check if number is approximately zero.
template<std::floating_point Real>
constexpr auto is_zero(Real a) noexcept -> bool {
  return abs(a) <= small_number_v<Real>;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the largest integer value not greater than number.
template<std::floating_point Real>
constexpr auto floor(Real a) noexcept -> Real {
  return std::floor(a);
}

/// Computes the nearest integer value to number.
template<std::floating_point Real>
constexpr auto round(Real a) noexcept -> Real {
  return std::round(a);
}

/// Compute the least integer value not less than number.
template<std::floating_point Real>
constexpr auto ceil(Real a) noexcept -> Real {
  return std::ceil(a);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inverse number.
/// @returns Floating-point reciprocal.
/// @{
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
/// @}

/// Divide numbers.
/// @returns Floating-point division result.
/// @{
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
/// @}

/// Safe inverse number.
/// @returns Inverse for non-zero input, zero for zero input.
template<std::floating_point Real>
constexpr auto safe_inverse(Real a) noexcept -> Real {
  return is_zero(a) ? Real{0.0} : inverse(a);
}

/// Safe divide number by divisor.
/// @returns Division result for non-zero divisor, zero if divisor is small.
template<class Num, std::floating_point Real>
constexpr auto safe_divide(Num a, Real b) noexcept {
  return is_zero(b) ? div_result_t<Num, Real>{} : a / b;
}

/// Ceiling divide unsigned integer.
template<std::unsigned_integral UInt>
constexpr auto ceil_divide(UInt a, UInt b) noexcept -> UInt {
  return (a + b - UInt{1}) / b;
}

/// Align unsigned integer.
template<std::unsigned_integral UInt>
constexpr auto align(UInt a, UInt alignment) noexcept -> UInt {
  return ceil_divide(a, alignment) * alignment;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Raise to the second power with 1 multiplication.
template<class Num>
constexpr auto pow2(Num a) noexcept -> Num {
  return a * a;
}

/// Raise to the third power with 2 multiplications.
template<class Num>
constexpr auto pow3(Num a) noexcept -> Num {
  return a * a * a;
}

/// Raise to the fourth power with 2 multiplications.
template<class Num>
constexpr auto pow4(Num a) noexcept -> Num {
  auto const a_sqr = a * a;
  return a_sqr * a_sqr;
}

/// Raise to the fifth power with 3 multiplications.
template<class Num>
constexpr auto pow5(Num a) noexcept -> Num {
  auto const a_sqr = a * a;
  return a_sqr * a_sqr * a;
}

/// Raise to the sixth power with 3 multiplications.
template<class Num>
constexpr auto pow6(Num a) noexcept -> Num {
  auto const a_cubed = a * a * a;
  return a_cubed * a_cubed;
}

/// Raise to the seventh power with 4 multiplications.
template<class Num>
constexpr auto pow7(Num a) noexcept -> Num {
  // TODO: can `pow7` be implemented with 3 multiplications?
  auto const a_cubed = a * a * a;
  return a_cubed * a_cubed * a;
}

/// Raise to the eighth power with 3 multiplications.
template<class Num>
constexpr auto pow8(Num a) noexcept -> Num {
  auto const a_sqr = a * a;
  auto const a_pow4 = a_sqr * a_sqr;
  return a_pow4 * a_pow4;
}

/// Raise to the ninth power with 4 multiplications.
template<class Num>
constexpr auto pow9(Num a) noexcept -> Num {
  auto const a_cubed = a * a * a;
  return a_cubed * a_cubed * a_cubed;
}

/// Raise to the power.
template<class Num>
constexpr auto pow(Num a, std::type_identity_t<Num> power) noexcept -> Num {
  return std::pow(a, power);
}

/// Square root.
template<std::floating_point Real>
constexpr auto sqrt(Real a) noexcept -> Real {
  return std::sqrt(a);
}

/// Cube root.
template<std::floating_point Real>
constexpr auto cbrt(Real a) noexcept -> Real {
  return std::cbrt(a);
}

/// Hypot.
/// @{
template<std::floating_point Real>
constexpr auto hypot(Real a, Real b) noexcept -> Real {
  return std::hypot(a, b);
}
template<std::floating_point Real>
constexpr auto hypot(Real a, Real b, Real c) noexcept -> Real {
  return std::hypot(a, b, c);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exponent.
template<std::floating_point Real>
constexpr auto exp(Real a) noexcept -> Real {
  return std::exp(a);
}

/// Exponent base two.
/// @{
template<std::floating_point Real>
constexpr auto exp2(Real a) noexcept -> Real {
  return std::exp2(a);
}
template<std::unsigned_integral UInt>
constexpr auto exp2(UInt a) noexcept -> UInt {
  return UInt{1} << a;
}
/// @}

/// Logarithm.
template<std::floating_point Real>
constexpr auto log(Real a) noexcept -> Real {
  return std::log(a);
}

/// Logarithm base two.
/// @{
template<std::floating_point Real>
constexpr auto log2(Real a) noexcept -> Real {
  return std::log2(a);
}
template<std::unsigned_integral UInt>
constexpr auto log2(UInt a) noexcept -> UInt {
  TIT_ASSERT(a != 0, "Cannot take base-2 logarithm of zero.");
  return std::bit_width(a) - UInt{1};
}
/// @}

/// Check if integer a is power of two.
template<std::unsigned_integral UInt>
constexpr auto is_power_of_two(UInt a) noexcept -> bool {
  return (a & (a - UInt{1})) == UInt{0};
}

/// Align-up integer to the nearest power of two.
template<std::unsigned_integral UInt>
constexpr auto align_to_power_of_two(UInt a) noexcept -> UInt {
  // TODO: maybe a branchless implementation?
  return is_power_of_two(a) ? a : exp2(log2(a) + 1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Arithmetic average function.
/// @note Presence of infinities of different signs will generate NaN.
template<class... Nums>
  requires (sizeof...(Nums) > 0)
constexpr auto avg(Nums... values) noexcept {
  return divide((values + ...), sizeof...(Nums));
}

/// Harmonic average function.
/// @param values Input values. Must be positive.
template<std::floating_point... Reals>
  requires (sizeof...(Reals) > 0)
constexpr auto havg(Reals... values) noexcept {
  TIT_ASSERT(((values >= 0) && ...),
             "Harmonic average requires all non-negative input.");
  return sizeof...(Reals) / (inverse(values) + ...);
}

/// Geometric average functions.
/// @param values Input values. Must be positive.
template<std::floating_point... Reals>
  requires (sizeof...(Reals) > 0)
constexpr auto gavg(Reals... values) noexcept {
  TIT_ASSERT(((values >= 0) && ...),
             "Geometric average requires all non-negative input.");
  return pow((values * ...), inverse(sizeof...(Reals)));
}

/// @brief Merge number with zero vector based on condition.
/// Implementation of this function is supposed to be branchless.
///
/// @note This function does not propagate floating-point infinites or NaNs
///       from @p a condition is `false`.
template<class Num>
  requires std::is_trivial_v<Num>
constexpr auto merge(bool m, Num a) noexcept -> Num {
  // Supposed to be overridden by intrinsics or optimized-out.
  // TODO: implement with bit operations.
  return m ? a : Num{0};
}

/// @brief Select one of two numbers vector based on condition.
/// Implementation of this function is supposed to be branchless.
///
/// @note This function does not propagate floating-point infinites or NaNs
///       from the argument which was not selected.
template<class Num>
  requires std::is_trivial_v<Num>
constexpr auto merge(bool m, Num a, Num b) noexcept -> Num {
  // Supposed to be overridden by intrinsics or optimized-out.
  // TODO: implement with bit operations.
  return m ? a : b;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Find function rool using Newton-Raphson method.
/// @param x Current estimate of the root.
/// @param f Function whose root we are looking for.
///          Should return a pair of it's value and derivative.
///          Should implicitly depend on @p x
/// @param eps Tolerance.
/// @param max_iter Maximum amount of iterations.
/// @returns True on success.
template<std::floating_point Real, std::invocable Func>
constexpr auto newton_raphson(Real& x, Func const& f, //
                              Real eps = Real{1.0e-9},
                              size_t max_iter = 10) -> bool {
  for (size_t iter = 0; iter < max_iter; ++iter) {
    auto const [y, df_dx] = std::invoke(f /*, x*/);
    if (abs(y) <= eps) return true;
    if (is_zero(df_dx)) break;
    x -= y / df_dx;
  }
  return false;
}

/// Find function rool using Bisection method.
/// @param min_x Root's lower bound.
/// @param max_x Root's upper bound.
/// @param f Function whose root we are looking for.
/// @param eps Tolerance.
/// @param max_iter Maximum amount of iterations.
/// @returns True on success.
template<std::floating_point Real, std::invocable<Real> Func>
constexpr auto bisection(Real& min_x, Real& max_x, Func const& f,
                         Real eps = Real{1.0e-9},
                         size_t max_iter = 10) -> bool {
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
    Real const x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    auto const y = std::invoke(f, x);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
