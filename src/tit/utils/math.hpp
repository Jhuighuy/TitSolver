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

#include <cmath>
#include <concepts>
#include <limits>
#include <stdexcept>

#include "tit/utils/assert.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Real type. */
using real_t = double;

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

/** Dimension type. */
using dim_t = int;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using std::abs;

/** Sign function. */
template<class Num>
constexpr int sign(Num value) noexcept {
  return int(Num{0} < value) - int(value < Num{0});
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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using std::exp;
using std::log;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using std::hypot;
using std::pow;
using std::sqrt;

template<class Num>
constexpr Num pow2(Num value) noexcept {
  return pow(value, 2);
}
template<class Num>
constexpr Num pow3(Num value) noexcept {
  return pow(value, 3);
}
template<class Num>
constexpr Num pow4(Num value) noexcept {
  return pow(value, 4);
}
template<class Num>
constexpr Num pow5(Num value) noexcept {
  return pow(value, 5);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check if number is approximately zero. */
template<class Num>
  requires std::integral<Num> || std::floating_point<Num>
constexpr bool is_zero(Num value) noexcept {
  if constexpr (std::integral<Num>) {
    return value == Num{};
  } else if constexpr (std::floating_point<Num>) {
    return value * value < std::numeric_limits<Num>::epsilon();
  }
}

/** Positive value or zero. */
template<class Num>
constexpr Num positive(Num value) noexcept {
  return std::max(Num{}, value);
}
/** Negative value or zero. */
template<class Num>
constexpr Num negative(Num value) noexcept {
  return std::min(Num{}, value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Inverse number. */
template<class Real>
constexpr Real inverse(Real value) noexcept {
  TIT_ASSERT(!is_zero(value), "Cannot invert zero!");
  return Real{1.0} / value;
}

/** Divide number by divisor. */
template<class Num, class Real>
constexpr Real divide(Num value, Real divisor) noexcept {
  TIT_ASSERT(!is_zero(divisor), "Cannot divide by zero!");
  return value / divisor;
}

/** Safe inverse number.
 ** @returns Inverse for non-zero input, zero for zero input. */
template<class Real>
constexpr Real safe_inverse(Real value) noexcept {
  return is_zero(value) ? Real{0.0} : inverse(value);
}

/** Safe divide number by divisor.
 ** @returns Division result for non-zero divisor, zero for zero divisor. */
template<class Num, class Real>
constexpr auto safe_divide(Num value, Real divisor) noexcept {
  return is_zero(divisor) ? div_result_t<Num, Real>{} : value / divisor;
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
