/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <cmath>
#include <concepts>
#include <initializer_list>
#include <limits>
#include <ranges>
#include <type_traits>

#ifdef __clang__
#include <gcem.hpp> // IWYU pragma: keep
#endif

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Clang does not have constexpr implementations of the math functions yet.
#ifndef __clang__
using std::abs;
using std::atan2;
using std::ceil;
using std::cos;
using std::exp;
using std::floor;
using std::log;
using std::round;
using std::sin;
using std::sqrt;
#else
#define TIT_MAKE_CONSTEXPR_MATH_FUNC_(func)                                    \
  constexpr auto func(std::floating_point auto... args) noexcept {             \
    if consteval {                                                             \
      return gcem::func(args...);                                              \
    }                                                                          \
    return std::func(args...);                                                 \
  }
TIT_MAKE_CONSTEXPR_MATH_FUNC_(abs)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(atan2)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(ceil)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(cos)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(exp)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(floor)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(log)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(round)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(sin)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(sqrt)
#undef TIT_MAKE_CONSTEXPR_MATH_FUNC_
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zero of the same type.
template<class Num>
constexpr auto zero(const Num& /*a*/) -> Num {
  return Num{0};
}

/// Square a number.
template<class Num>
constexpr auto pow2(Num a) -> Num {
  return a * a;
}

/// Raise to the unsigned integer power.
template<unsigned Power, class Num>
constexpr auto pow(Num a) -> Num {
  static_assert(Power > 0, "Power must be positive!");
  if constexpr (Power == 1) return a;
  else if constexpr (Power % 2 == 0) return pow<Power / 2>(a * a);
  else return a * pow<Power / 2>(a * a);
}

/// Raise to the floating-point power.
template<std::floating_point Float>
constexpr auto pow(Float a, std::type_identity_t<Float> power) noexcept
    -> Float {
#ifdef __clang__
  if consteval {
    return gcem::pow(a, power);
  }
#endif
  return std::pow(a, power);
}

/// Evaluate polynomial @f$ \sum c_k x^k @f$ value.
template<class Num>
constexpr auto horner(Num x, std::initializer_list<Num> ci) {
  Num r{0};
  for (const auto c : ci | std::views::reverse) r = r * x + c;
  return r;
}

/// Number reciprocal.
template<class Num>
constexpr auto inverse(Num a) -> Num {
  return Num{1} / a;
}

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
inline constexpr auto tiny_v = Num{};

/// @todo Can we switch to a more precise tiny number, e.g. `sqrt(epsilon())`?
template<std::floating_point Float>
inline constexpr auto tiny_v<Float> =
    pow(std::numeric_limits<Float>::epsilon(), Float{1.0 / 3.0});

/// Check if number is tiny.
template<class Num>
constexpr auto is_tiny(Num a) noexcept -> bool {
  return abs(a) <= tiny_v<Num>;
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
  using Bits =
      std::conditional_t<sizeof(Num) == sizeof(uint32_t), uint32_t, uint64_t>;
  return std::bit_cast<Bits>(a) == std::bit_cast<Bits>(b);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
