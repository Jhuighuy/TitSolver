/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/math.hpp"
#pragma once

#include <cmath>
#include <concepts>
#include <initializer_list>
#include <limits>
#include <ranges>
#include <type_traits>

#include "tit/core/type_traits.hpp"

namespace tit {

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tiny number, used a systemwise zero tolerance.
template<class Num>
inline constexpr auto tiny_number_v = Num{};

template<std::floating_point Float>
inline constexpr auto tiny_number_v<Float>{
#ifdef __clang__ // clang's `cbrt` is not constexpr yet.
    std::numeric_limits<Float>::epsilon()
#else
    std::cbrt(std::numeric_limits<Float>::epsilon())
#endif
};

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
