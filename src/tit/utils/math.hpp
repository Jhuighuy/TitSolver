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
#include <limits>

#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <utility>

#include "tit/utils/assert.hpp"
#include "tit/utils/config.hpp"
#include "tit/utils/misc.hpp"

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
using dim_t = ptrdiff_t;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using std::abs;

/** Sign function. */
template<class Num>
constexpr auto sign(Num value) noexcept -> int {
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

using std::cbrt;
using std::hypot;
using std::pow;
using std::sqrt;

template<class Num>
constexpr auto pow2(Num value) noexcept -> Num {
  // 1 multiplication.
  return value * value;
}
template<class Num>
constexpr auto pow3(Num value) noexcept -> Num {
  // 2 multiplications.
  return value * value * value;
}
template<class Num>
constexpr auto pow4(Num value) noexcept -> Num {
  // 2 multiplications.
  const auto value_sqr = value * value;
  return value_sqr * value_sqr;
}
template<class Num>
constexpr auto pow5(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_sqr = value * value;
  return value_sqr * value_sqr * value;
}
template<class Num>
constexpr auto pow6(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_cube = value * value * value;
  return value_cube * value_cube;
}
template<class Num>
constexpr auto pow7(Num value) noexcept -> Num {
  // 4 multiplications.
  const auto value_cube = value * value * value;
  return value_cube * value_cube * value;
}
template<class Num>
constexpr auto pow8(Num value) noexcept -> Num {
  // 3 multiplications.
  const auto value_sqr = value * value;
  const auto value_sqr_sqr = value_sqr * value_sqr;
  return value_sqr_sqr * value_sqr_sqr;
}
template<class Num>
constexpr auto pow9(Num value) noexcept -> Num {
  // 4 multiplications.
  const auto value_cube = value * value * value;
  return value_cube * value_cube * value_cube;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check if number is approximately zero. */
template<class Num>
  requires std::integral<Num> || std::floating_point<Num>
constexpr auto is_zero(Num value) noexcept -> bool {
  if constexpr (std::integral<Num>) {
    return value == Num{};
  } else if constexpr (std::floating_point<Num>) {
    return value * value < std::numeric_limits<Num>::epsilon();
  }
}

/** Positive value or zero. */
template<class Num>
constexpr auto positive(Num value) noexcept -> Num {
  return std::max(Num{}, value);
}
/** Negative value or zero. */
template<class Num>
constexpr auto negative(Num value) noexcept -> Num {
  return std::min(Num{}, value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Inverse number. */
template<class Num>
constexpr auto inverse(Num value) noexcept -> Num {
  TIT_ASSERT(!is_zero(value), "Cannot invert zero!");
  return Num{1} / value;
}

/** Divide number by divisor. */
template<class Num, class Real>
constexpr auto divide(Num value, Real divisor) noexcept {
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

#if 0
/** Merge floating-point number with zero vector based on condition. */
template<std::floating_point Float>
constexpr Float merge(bool b, Num t) noexcept -> Float {
  if consteval {
    return b * t;
  } else {
    // Explicitly tell the compiler that multiplication by boolean
    // is a bitwise and operation rather then real multiplication.
    using mask_t = decltype([] {
      if constexpr (sizeof(Float) == 1) return uint8_t{};
      else if constexpr (sizeof(Float) == 2) return uint16_t{};
      else if constexpr (sizeof(Float) == 4) return uint32_t{};
      else if constexpr (sizeof(Float) == 8) return uint64_t{};
      else static_assert(false, "Floating point type is too long.");
    }());
    const auto mask = b ? ~mask_t{0} : mask_t{0};
    return union_cast<Float>(mask & union_cast<mask_t>(t));
  }
}
#endif

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
