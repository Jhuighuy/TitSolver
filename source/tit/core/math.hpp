/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <cmath>
#include <concepts>
#include <expected>
#include <functional>
#include <initializer_list>
#include <limits>
#include <ranges>
#include <type_traits>

#ifdef __clang__
#include <gcem.hpp> // IWYU pragma: keep
#endif

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef __clang__

// GCC already has constexpr implementations of the math functions.
using std::abs;
using std::atan2;
using std::cbrt;
using std::ceil;
using std::cos;
using std::exp;
using std::floor;
using std::log;
using std::round;
using std::sin;
using std::sqrt;

#else

// Clang does not have constexpr implementations of the math functions yet.
#define TIT_MAKE_CONSTEXPR_MATH_FUNC(func)                                     \
  template<std::floating_point Float>                                          \
  constexpr auto func(Float a) noexcept -> Float {                             \
    if consteval {                                                             \
      return gcem::func(a);                                                    \
    }                                                                          \
    return std::func(a);                                                       \
  }
TIT_MAKE_CONSTEXPR_MATH_FUNC(abs)
TIT_MAKE_CONSTEXPR_MATH_FUNC(ceil)
TIT_MAKE_CONSTEXPR_MATH_FUNC(cos)
TIT_MAKE_CONSTEXPR_MATH_FUNC(exp)
TIT_MAKE_CONSTEXPR_MATH_FUNC(floor)
TIT_MAKE_CONSTEXPR_MATH_FUNC(log)
TIT_MAKE_CONSTEXPR_MATH_FUNC(round)
TIT_MAKE_CONSTEXPR_MATH_FUNC(sin)
TIT_MAKE_CONSTEXPR_MATH_FUNC(sqrt)
#undef TIT_MAKE_CONSTEXPR_MATH_FUNC

template<std::floating_point Float>
constexpr auto cbrt(Float a) noexcept -> Float {
  if consteval {
    return gcem::pow(a, Float{1.0 / 3.0});
  }
  return std::cbrt(a);
}

template<std::floating_point Float>
constexpr auto atan2(Float a, Float b) noexcept -> Float {
  if consteval {
    return gcem::atan2(a, b);
  }
  return std::atan2(a, b);
}

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zero of the same type.
template<class Num>
constexpr auto zero(const Num& /*a*/) -> Num {
  return Num{0};
}

/// Sign of the value.
template<class Num>
constexpr auto sign(Num a) -> Num {
  return static_cast<Num>(int{Num{0} < a} - int{a < Num{0}});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Raise to the second power with 1 multiplication.
template<class Num>
constexpr auto pow2(Num a) -> Num {
  return a * a;
}

/// Raise to the third power with 2 multiplications.
template<class Num>
constexpr auto pow3(Num a) -> Num {
  return a * a * a;
}

/// Raise to the fourth power with 2 multiplications.
template<class Num>
constexpr auto pow4(Num a) -> Num {
  const auto a_sqr = a * a;
  return a_sqr * a_sqr;
}

/// Raise to the fifth power with 3 multiplications.
template<class Num>
constexpr auto pow5(Num a) -> Num {
  const auto a_sqr = a * a;
  return a_sqr * a_sqr * a;
}

/// Raise to the sixth power with 3 multiplications.
template<class Num>
constexpr auto pow6(Num a) -> Num {
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed;
}

/// Raise to the seventh power with 4 multiplications.
template<class Num>
constexpr auto pow7(Num a) -> Num {
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed * a;
}

/// Raise to the eighth power with 3 multiplications.
template<class Num>
constexpr auto pow8(Num a) -> Num {
  const auto a_sqr = a * a;
  const auto a_pow4 = a_sqr * a_sqr;
  return a_pow4 * a_pow4;
}

/// Raise to the ninth power with 4 multiplications.
template<class Num>
constexpr auto pow9(Num a) -> Num {
  const auto a_cubed = a * a * a;
  return a_cubed * a_cubed * a_cubed;
}

/// Raise to the power.
template<std::floating_point Float>
constexpr auto pow(Float a,
                   std::type_identity_t<Float> power) noexcept -> Float {
#ifdef __clang__
  if consteval {
    return gcem::pow(a, power);
  }
#endif
  return std::pow(a, power);
}

/// Evaluate polynomial @f$ \sum c_k x^k @f$ value.
template<class Num, class Coeff>
constexpr auto horner(Num x, std::initializer_list<Coeff> ci) {
  mul_result_t<Num, Coeff> r{0};
  for (const auto c : ci | std::views::reverse) r = r * x + c;
  return r;
}

/// Number reciprocal.
template<class Num>
constexpr auto inverse(Num a) -> Num {
  return Num{1} / a;
}

/// Square root reciprocal.
template<std::floating_point Float>
constexpr auto rsqrt(Float a) noexcept -> Float {
  return Float{1.0} / std::sqrt(a);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Arithmetic average function.
template<class... Nums>
  requires (sizeof...(Nums) > 0)
constexpr auto avg(Nums... vals) {
  return (vals + ...) / sizeof...(Nums);
}

/// Harmonic average function.
template<class... Nums>
  requires (sizeof...(Nums) > 0)
constexpr auto havg(Nums... vals) noexcept {
  return sizeof...(Nums) / (inverse(vals) + ...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tiny number, used a systemwise zero tolerance.
template<class Num>
inline constexpr auto tiny_number_v = Num{};

template<std::floating_point Float>
inline constexpr auto tiny_number_v<Float> =
    cbrt(std::numeric_limits<Float>::epsilon());

/// Check if number is tiny.
template<class Num>
constexpr auto is_tiny(Num a) noexcept -> bool {
  return abs(a) <= tiny_number_v<Num>;
}

/// Check if two numbers are approximately equal.
template<class Num>
constexpr auto approx_equal_to(Num a, Num b) -> bool {
  return is_tiny(a - b);
}

/// Check if two numbers are bitwise equal.
///
/// This function may be used to as a fast comparison of floating-point numbers
/// that are known to be not NaN.
template<std::floating_point Num>
[[gnu::always_inline]]
constexpr auto bitwise_equal(Num a, Num b) noexcept -> bool {
  if constexpr (sizeof(Num) == sizeof(uint32_t)) {
    return std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b);
  } else if constexpr (sizeof(Num) == sizeof(uint64_t)) {
    return std::bit_cast<uint64_t>(a) == std::bit_cast<uint64_t>(b);
  } else static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Newton-Raphson solver error type.
enum class NewtonError : uint8_t {
  no_error,      ///< No error.
  not_converged, ///< Number of iterations exceeded.
  zero_deriv,    ///< Computation reached a zero derivative.
};

/// Newton-Raphson solver result type.
template<class Num>
using NewtonResult = std::expected<Num, NewtonError>;

/// Find function root using the Newton-Raphson method.
template<std::floating_point Num, class Func>
  requires std::invocable<Func&&, Num>
constexpr auto newton(Num x,
                      Func&& f,
                      std::type_identity_t<Num> eps = tiny_number_v<Num>,
                      size_t max_iter = 10) -> NewtonResult<Num> {
  TIT_ASSUME_UNIVERSAL(Func, f);
  for (size_t iter = 0; iter < max_iter; ++iter) {
    const auto [y, df_dx] = std::invoke(f, x);
    if (abs(y) <= eps) return x;
    if (is_tiny(df_dx)) return std::unexpected{NewtonError::zero_deriv};
    x -= y / df_dx;
  }
  return std::unexpected{NewtonError::not_converged};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bisection solver error type.
enum class BisectionError : uint8_t {
  no_error,      ///< No error.
  not_converged, ///< Number of iterations exceeded.
  sign,          ///< Function has same signs on the ends of the search range.
};

/// Bisection solver result type.
template<class Num>
using BisectionResult = std::expected<Num, BisectionError>;

/// Find function root using the Bisection method.
template<class Num, class Func>
  requires std::invocable<Func&&, Num>
constexpr auto bisection(Num min_x,
                         Num max_x,
                         Func&& f,
                         std::type_identity_t<Num> eps = tiny_number_v<Num>,
                         size_t max_iter = 10) -> BisectionResult<Num> {
  TIT_ASSUME_UNIVERSAL(Func, f);

  // Check the search bounds first.
  TIT_ASSERT(min_x <= max_x, "Inverted search range!");
  auto min_f = std::invoke(f, min_x);
  if (abs(min_f) <= eps) return min_x;
  auto max_f = std::invoke(f, max_x);
  if (abs(max_f) <= eps) return max_x;

  for (size_t iter = 0; iter < max_iter; ++iter) {
    if (sign(max_f) == sign(min_f)) {
      return std::unexpected{BisectionError::sign};
    }

    // Approximate f(x) with line equation:
    // f(x) = min_f + (max_f - min_f)/(max_x - min_x) * (x - min_x),
    // therefore, approximate root of f(x) == 0 is:
    const auto x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    const auto y = std::invoke(f, x);
    if (abs(y) <= eps) return x;

    // Update the search bounds.
    const auto sign_y = sign(y);
    if (sign_y != sign(min_f)) max_x = x, max_f = y;
    else if (sign_y != sign(max_f)) min_x = x, min_f = y;
  }

  return std::unexpected{BisectionError::not_converged};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
