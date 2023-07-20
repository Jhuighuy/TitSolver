/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <cmath>
#include <limits>

#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <type_traits>
#include <utility>

#include "tit/utils/assert.hpp"
#include "tit/utils/types.hpp"

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
constexpr auto abs(Num value) noexcept -> Num {
  return std::abs(value);
}
/** Sign of the value. */
template<class Num>
constexpr auto sign(Num value) noexcept -> int {
  return int(Num{0} < value) - int(value < Num{0});
}

/** Small number, treated as zero. */
template<class Real>
  requires std::floating_point<Real>
constexpr auto small_number_v = sqrt(std::numeric_limits<Real>::epsilon());

/** Check if number is approximately zero. */
template<class Real>
constexpr auto is_zero(Real value) noexcept -> bool {
  return abs(value) < small_number_v<Real>;
}
/** Check if numbers are approximately equal. */
template<class Real>
constexpr auto approx_equal(Real a, Real b) noexcept -> bool {
  return is_zero(a - b);
}

/** Positive value or zero. */
template<class Num>
constexpr auto plus(Num value) noexcept -> Num {
  return std::max(Num{0}, value);
}
/** Negative value or zero. */
template<class Num>
constexpr auto minus(Num value) noexcept -> Num {
  return std::min(Num{0}, value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Inverse number. */
template<class Real>
constexpr auto inverse(Real value) noexcept -> Real {
  TIT_ASSERT(!is_zero(value), "Cannot invert zero!");
  return Real{1.0} / value;
}

/** Safe inverse number.
 ** @returns Inverse for non-zero input, zero for zero input. */
template<class Real>
constexpr auto safe_inverse(Real value) noexcept -> Real {
  return is_zero(value) ? Real{0.0} : inverse(value);
}
/** Safe divide number by divisor.
 ** @returns Division result for non-zero divisor, zero for zero divisor. */
template<class Num, class Real>
constexpr auto safe_divide(Num value, Real divisor) noexcept {
  return is_zero(divisor) ? div_result_t<Num, Real>{} : value / divisor;
}

/** Ceiling divide unsigned integer. */
template<std::unsigned_integral Int>
constexpr auto ceil_divide(Int value, Int divisor) noexcept -> Int {
  return (value + divisor - 1u) / divisor;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Raise to the second power. */
template<class Num>
constexpr auto pow2(Num value) noexcept -> Num {
  // 1 multiplication.
  return value * value;
}
/** Raise to the third power. */
template<class Num>
constexpr auto pow3(Num value) noexcept -> Num {
  // 2 multiplications.
  return value * value * value;
}
/** Raise to the fourth power. */
template<class Num>
constexpr auto pow4(Num value) noexcept -> Num {
  // 2 multiplications.
  const auto value_sqr = value * value;
  return value_sqr * value_sqr;
}
/** Raise to the fifth power. */
template<class Num>
constexpr auto pow5(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_sqr = value * value;
  return value_sqr * value_sqr * value;
}
/** Raise to the sixth power. */
template<class Num>
constexpr auto pow6(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_cubed = value * value * value;
  return value_cubed * value_cubed;
}
/** Raise to the seventh power. */
template<class Num>
constexpr auto pow7(Num value) noexcept -> Num {
  // 4 multiplications.
  // TODO: can pow7 be implemented with 3 multiplications?
  const auto value_cubed = value * value * value;
  return value_cubed * value_cubed * value;
}
/** Raise to the eighth power. */
template<class Num>
constexpr auto pow8(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_sqr = value * value;
  const auto value_pow4 = value_sqr * value_sqr;
  return value_pow4 * value_pow4;
}
/** Raise to the nineth power. */
template<class Num>
constexpr auto pow9(Num value) noexcept -> Num {
  // 4 multiplications.
  const auto value_cubed = value * value * value;
  return value_cubed * value_cubed * value_cubed;
}
/** Raise to power. */
template<class Num>
constexpr auto pow(Num value, std::type_identity_t<Num> power) noexcept {
  return std::pow(value, power);
}

/** Square root. */
template<class Num>
constexpr auto sqrt(Num value) noexcept {
  return std::sqrt(value);
}

/** Cube root. */
template<class Num>
constexpr auto cbrt(Num value) noexcept {
  return std::cbrt(value);
}

/** Hypot. */
/** @{ */
template<class Num>
constexpr auto hypot(Num x, Num y) noexcept {
  return std::hypot(x, y);
}
template<class Num>
constexpr auto hypot(Num x, Num y, Num z) noexcept {
  return std::hypot(x, y, z);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Exponent. */
template<class Num>
constexpr auto exp(Num value) noexcept -> Num {
  return std::exp(value);
}
/** Logarithm. */
template<class Num>
constexpr auto log(Num value) noexcept -> Num {
  return std::log(value);
}

/** Integer exponent base-2. */
template<std::unsigned_integral Int>
constexpr auto exp2(Int x) noexcept -> Int {
  return Int{1u} << x;
}
/** Integer logarithm base-2. */
template<std::unsigned_integral Int>
constexpr auto log2(Int x) noexcept -> Int {
  return std::bit_width(x) - Int{1u};
}
/** Check if integer value is power of two. */
template<std::unsigned_integral Int>
constexpr auto is_power_of_two(Int x) noexcept -> bool {
  return (x & (x - Int{1u})) == 0;
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

/** Merge number with zero vector based on condition. */
template<class Num>
constexpr auto merge(bool m, Num a) noexcept {
  // Supposed to be overriden by intrisics or optimized.
  return m * a;
}
/** Merge two numbers with zero vector based on condition. */
template<class NumA, class NumB>
constexpr auto merge(bool m, NumA a, NumB b) noexcept {
  // Supposed to be overriden by intrisics or optimized.
  return m * a + (!m) * b;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Find function rool using Newton-Raphson method.
 ** @returns True on success. */
template<class Real, class Func>
  requires std::invocable<Func>
constexpr bool newton_raphson(Real& x, const Func& f, //
                              Real epsilon = Real{1e-9}, size_t max_iter = 10) {
  for (size_t iter = 0; iter < max_iter; ++iter) {
    const auto [y, df_dx] = f(/*x*/);
#if 0
    std::cout << "NR: i = " << iter             //
              << ", x = " << x << ", y = " << y //
              << ", df/dx = " << df_dx << std::endl;
#endif
    if (abs(y) < epsilon) return true;
    if (is_zero(df_dx)) break;
    x -= y / df_dx;
  }
  return false;
}

/** Find function rool using Bisection method.
 ** @returns True on success. */
template<class Real, class Func>
  requires std::invocable<Func, Real>
constexpr bool bisection(Real& min_x, Real& max_x, const Func& f,
                         Real epsilon = Real{1e-9}, size_t max_iter = 10) {
  TIT_ASSERT(min_x <= max_x, "Inverted search range!");
  auto min_f = f(min_x);
  if (abs(min_f) < epsilon) {
    max_x = min_x;
    return true;
  }
  auto max_f = f(max_x);
  if (abs(max_f) < epsilon) {
    min_x = max_x;
    return true;
  }
  if (sign(max_f) == sign(min_f)) return false;
  for (size_t iter = 0; iter < max_iter; ++iter) {
    // Approximate f(x) with line equation:
    // f(x) = min_f + (max_f - min_f)/(max_x - min_x) * (x - min_x),
    // so approximate root of f(x) == 0 is:
    const Real x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    const auto y = f(x);
    if (abs(y) == epsilon) {
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
