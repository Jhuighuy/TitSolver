/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
#include "tit/core/checks.hpp"

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
using std::log10;
using std::log2;
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
TIT_MAKE_CONSTEXPR_MATH_FUNC_(log10)
TIT_MAKE_CONSTEXPR_MATH_FUNC_(log2)
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

/// Raise to the non-negative integer power.
/// @{
template<class Num>
constexpr auto ipow(Num a, std::integral auto power) -> Num {
  if constexpr (std::integral<Num> || std::floating_point<Num>) {
    TIT_ASSERT(power >= 0, "Power must be non-negative!");
    if (power == 0) return Num{1};
  } else TIT_ASSERT(power > 0, "Power must be positive!");
  if (power == 1) return a;
  if (power % 2 == 0) return ipow(a * a, power / 2);
  return a * ipow(a * a, power / 2);
}
template<std::integral auto Power, class Num>
constexpr auto pow(Num a) -> Num {
  return ipow(a, Power);
}
/// @}

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

/// Default value is ~6.0e-6.
template<std::floating_point Float>
inline constexpr auto tiny_v<Float> =
    static_cast<Float>(pow(std::numeric_limits<double>::epsilon(), 1.0 / 3.0));

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
