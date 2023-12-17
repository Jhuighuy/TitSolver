/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <limits>
#include <numbers> // IWYU pragma: keep

#include <doctest/doctest.h>

#include "tit/core/config.hpp"
#include "tit/core/math.hpp"

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::abs", T, int, float, double) {
  CHECK(tit::abs(T{0}) == T{0});
  CHECK(tit::abs(+T{2}) == T{2});
  CHECK(tit::abs(-T{2}) == T{2});
}

TEST_CASE_TEMPLATE("tit::core::plus", T, int, float, double) {
  CHECK(tit::plus(T{0}) == T{0});
  CHECK(tit::plus(+T{2}) == T{2});
  CHECK(tit::plus(-T{2}) == T{0});
}

TEST_CASE_TEMPLATE("tit::core::minus", T, int, float, double) {
  CHECK(tit::minus(T{0}) == T{0});
  CHECK(tit::minus(+T{2}) == T{0});
  CHECK(tit::minus(-T{2}) == -T{2});
}

TEST_CASE_TEMPLATE("tit::core::sign", T, int, float, double) {
  CHECK(tit::sign(T{0}) == T{0});
  CHECK(tit::sign(+T{2}) == +T{1});
  CHECK(tit::sign(-T{2}) == -T{1});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::small_number_v", T, float, double) {
  // Small number must be positive.
  CHECK(tit::small_number_v<T> > 0.0);
  // Small number should be larger than machine epsilon.
  CHECK(tit::small_number_v<T> >= std::numeric_limits<double>::epsilon());
}

TEST_CASE_TEMPLATE("tit::core::is_zero", T, float, double) {
  // Check ordinary numbers.
  CHECK(tit::is_zero(+T{0.0}));
  CHECK(tit::is_zero(-T{0.0}));
  CHECK(!tit::is_zero(+T{1.0}));
  CHECK(!tit::is_zero(-T{1.0}));
  // Check if comparisons with `small_number_v` work as expected.
  CHECK(tit::is_zero(+tit::small_number_v<T>));
  CHECK(tit::is_zero(-tit::small_number_v<T>));
  CHECK(tit::is_zero(+T{0.1} * tit::small_number_v<T>));
  CHECK(tit::is_zero(-T{0.1} * tit::small_number_v<T>));
  CHECK(!tit::is_zero(+T{2.0} * tit::small_number_v<T>));
  CHECK(!tit::is_zero(-T{2.0} * tit::small_number_v<T>));
}

TEST_CASE_TEMPLATE("tit::core::approx_eq", T, float, double) {
  // Check ordinary numbers.
  CHECK(tit::approx_eq(T{1.234}, T{1.234}));
  CHECK(!tit::approx_eq(T{1.234}, T{5.5678}));
  // Check if comparisons with `small_number_v` work as expected.
  CHECK(tit::approx_eq(T{1.234}, T{1.234} + tit::small_number_v<T>));
  CHECK(tit::approx_eq(T{1.234} - tit::small_number_v<T>, T{1.234}));
  CHECK(tit::approx_eq(T{1.234}, T{1.234} + T{0.1} * tit::small_number_v<T>));
  CHECK(tit::approx_eq(T{1.234} - T{0.1} * tit::small_number_v<T>, T{1.234}));
  CHECK(!tit::approx_eq(T{1.234}, T{1.234} + T{2.0} * tit::small_number_v<T>));
  CHECK(!tit::approx_eq(T{1.234} - T{2.0} * tit::small_number_v<T>, T{1.234}));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::floor", T, float, double) {
  // Check non-negative numbers.
  CHECK(tit::floor(T{0.0}) == T{0.0});
  CHECK(tit::floor(T{1.1}) == T{1.0});
  CHECK(tit::floor(T{1.5}) == T{1.0});
  CHECK(tit::floor(T{1.9}) == T{1.0});
  // Check negative numbers.
  CHECK(tit::floor(-T{1.1}) == -T{2.0});
  CHECK(tit::floor(-T{1.5}) == -T{2.0});
  CHECK(tit::floor(-T{1.9}) == -T{2.0});
}

TEST_CASE_TEMPLATE("tit::core::round", T, float, double) {
  // Check non-negative numbers.
  CHECK(tit::round(T{0.0}) == T{0.0});
  CHECK(tit::round(T{1.1}) == T{1.0});
  CHECK(tit::round(T{1.5}) == T{2.0});
  CHECK(tit::round(T{1.9}) == T{2.0});
  // Check negative numbers.
  CHECK(tit::round(-T{1.1}) == -T{1.0});
  CHECK(tit::round(-T{1.5}) == -T{2.0});
  CHECK(tit::round(-T{1.9}) == -T{2.0});
}

TEST_CASE_TEMPLATE("tit::core::ceil", T, float, double) {
  // Check non-negative numbers.
  CHECK(tit::ceil(T{0.0}) == T{0.0});
  CHECK(tit::ceil(T{1.1}) == T{2.0});
  CHECK(tit::ceil(T{1.5}) == T{2.0});
  CHECK(tit::ceil(T{1.9}) == T{2.0});
  // Check negative numbers.
  CHECK(tit::ceil(-T{1.1}) == -T{1.0});
  CHECK(tit::ceil(-T{1.5}) == -T{1.0});
  CHECK(tit::ceil(-T{1.9}) == -T{1.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::inverse", T, int, float, double) {
  CHECK(tit::inverse(T{2}) == 0.5);
  CHECK(tit::inverse(T{8}) == 0.125);
}

TEST_CASE_TEMPLATE("tit::core::divide", T, int, float, double) {
  // Note: the result is always floating-point.
  CHECK(tit::divide(T{1}, 2) == 0.5);
  CHECK(tit::divide(T{1}, 2.0f) == 0.5);
  CHECK(tit::divide(T{1}, 2.0) == 0.5);
}

TEST_CASE_TEMPLATE("tit::core::safe_inverse", T, float, double) {
  // Check non-"small" numbers.
  CHECK(tit::safe_inverse(T{2.0}) == T{0.5});
  CHECK(tit::safe_inverse(T{10.0}) == T{0.1});
  // Check "small" numbers.
  CHECK(tit::safe_inverse(T{0.0}) == T{0.0});
  CHECK(tit::safe_inverse(tit::small_number_v<T>) == 0.0);
  CHECK(tit::safe_inverse(T{0.1} * tit::small_number_v<T>) == T{0.0});
  CHECK(tit::safe_inverse(T{2.0} * tit::small_number_v<T>) != T{0.0});
}

TEST_CASE_TEMPLATE("tit::core::safe_divide", T, float, double) {
  // Check non-"small" divisors.
  CHECK(tit::safe_divide(1, T{2.0}) == T{0.5});
  CHECK(tit::safe_divide(1, T{10.0}) == T{0.1});
  // Check "small" divisors.
  CHECK(tit::safe_divide(1, T{0.0}) == T{0.0});
  CHECK(tit::safe_divide(1, tit::small_number_v<T>) == T{0.0});
  CHECK(tit::safe_divide(1, T{0.1} * tit::small_number_v<T>) == T{0.0});
  CHECK(tit::safe_divide(1, T{2.0} * tit::small_number_v<T>) != T{0.0});
}

TEST_CASE_TEMPLATE("tit::core::ceil_divide", T, unsigned) {
  CHECK(tit::ceil_divide(T{0}, T{10}) == T{0});
  CHECK(tit::ceil_divide(T{3}, T{10}) == T{1});
  CHECK(tit::ceil_divide(T{7}, T{10}) == T{1});
  CHECK(tit::ceil_divide(T{10}, T{10}) == T{1});
  CHECK(tit::ceil_divide(T{11}, T{10}) == T{2});
  CHECK(tit::ceil_divide(T{20}, T{10}) == T{2});
}

TEST_CASE_TEMPLATE("tit::core::align", T, unsigned) {
  CHECK(tit::align(T{0}, T{10}) == T{0});
  CHECK(tit::align(T{3}, T{10}) == T{10});
  CHECK(tit::align(T{7}, T{10}) == T{10});
  CHECK(tit::align(T{10}, T{10}) == T{10});
  CHECK(tit::align(T{11}, T{10}) == T{20});
  CHECK(tit::align(T{20}, T{10}) == T{20});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::pow", T, int, float, double) {
  CHECK(tit::pow2(+T{2}) == +T{4});
  CHECK(tit::pow2(-T{2}) == +T{4});
  CHECK(tit::pow3(+T{2}) == +T{8});
  CHECK(tit::pow3(-T{2}) == -T{8});
  CHECK(tit::pow4(+T{2}) == +T{16});
  CHECK(tit::pow4(-T{2}) == +T{16});
  CHECK(tit::pow5(+T{2}) == +T{32});
  CHECK(tit::pow5(-T{2}) == -T{32});
  CHECK(tit::pow6(+T{2}) == +T{64});
  CHECK(tit::pow6(-T{2}) == +T{64});
  CHECK(tit::pow7(+T{2}) == +T{128});
  CHECK(tit::pow7(-T{2}) == -T{128});
  CHECK(tit::pow8(+T{2}) == +T{256});
  CHECK(tit::pow8(-T{2}) == +T{256});
  CHECK(tit::pow9(+T{2}) == +T{512});
  CHECK(tit::pow9(-T{2}) == -T{512});
  CHECK(tit::pow(+T{2}, 10) == T{1024});
  CHECK(tit::pow(-T{2}, 10) == T{1024});
}

TEST_CASE_TEMPLATE("tit::core::horner", T, int, float, double) {
  CHECK(tit::horner(T{1}, {T{1}}) == T{1});
  CHECK(tit::horner(T{3}, {T{1}, -T{3}, T{2}}) == T{10});
  CHECK(tit::horner(-T{2}, {T{4}, -T{1}, T{3}}) == T{18});
  CHECK(tit::horner(T{3}, {T{6}, T{1}, -T{4}, T{1}}) == T{0});
}

TEST_CASE_TEMPLATE("tit::core::sqrt", T, float, double) {
  CHECK(tit::sqrt(T{0.0}) == T{0.0});
  CHECK(tit::sqrt(T{4.0}) == T{2.0});
}

TEST_CASE_TEMPLATE("tit::core::cbrt", T, float, double) {
  CHECK(tit::cbrt(T{0.0}) == T{0.0});
  CHECK(tit::cbrt(+T{8.0}) == +T{2.0});
  CHECK(tit::cbrt(-T{8.0}) == -T{2.0});
}

TEST_CASE_TEMPLATE("tit::core::hypot", T, float, double) {
  CHECK(tit::hypot(T{3.0}, T{4.0}) == T{5.0});
  CHECK(tit::hypot(T{2.0}, T{6.0}, T{9.0}) == T{11.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::exp", T, float, double) {
  CHECK(tit::exp(T{0.0}) == T{1.0});
  CHECK(tit::approx_eq(tit::exp(T{1.0}), std::numbers::e_v<T>));
}

TEST_CASE_TEMPLATE("tit::core::exp2", T, unsigned, float, double) {
  CHECK(tit::exp2(T{0}) == T{1});
  CHECK(tit::exp2(T{1}) == T{2});
  CHECK(tit::exp2(T{9}) == T{512});
}

TEST_CASE_TEMPLATE("tit::core::log", T, float, double) {
  CHECK(tit::log(T{1.0}) == T{0.0});
  CHECK(tit::approx_eq(tit::log(std::numbers::e_v<T>), T{1.0}));
}

TEST_CASE_TEMPLATE("tit::core::log2", T, unsigned, float, double) {
  CHECK(tit::log2(T{1}) == T{0});
  CHECK(tit::log2(T{2}) == T{1});
  CHECK(tit::log2(T{512}) == T{9});
}

TEST_CASE_TEMPLATE("tit::core::is_power_of_two", T, unsigned) {
  CHECK(tit::is_power_of_two(T{0}));
  CHECK(tit::is_power_of_two(T{1}));
  CHECK(tit::is_power_of_two(T{512}));
  CHECK(!tit::is_power_of_two(T{255}));
  CHECK(!tit::is_power_of_two(T{513}));
}

TEST_CASE_TEMPLATE("tit::core::align_to_power_of_two", T, unsigned) {
  CHECK(tit::align_to_power_of_two(T{0}) == T{0});
  CHECK(tit::align_to_power_of_two(T{1}) == T{1});
  CHECK(tit::align_to_power_of_two(T{2}) == T{2});
  CHECK(tit::align_to_power_of_two(T{3}) == T{4});
  CHECK(tit::align_to_power_of_two(T{127}) == T{128});
  CHECK(tit::align_to_power_of_two(T{128}) == T{128});
  CHECK(tit::align_to_power_of_two(T{129}) == T{256});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::avg", T, int, float, double) {
  // Note: the result is always floating-point.
  CHECK(tit::avg(T{1}, T{2}) == 1.5);
  CHECK(tit::avg(T{1}, T{2}, T{3}) == 2.0);
}

TEST_CASE_TEMPLATE("tit::core::gavg", T, float, double) {
  CHECK(tit::havg(T{1.0}, T{4.0}) == T{1.6});
}

TEST_CASE_TEMPLATE("tit::core::gavg", T, float, double) {
  CHECK(tit::gavg(T{1.0}, T{4.0}) == T{2.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::core::merge", T, float, double) {
  CHECK(tit::merge(true, T{2.0}) == T{2.0});
  CHECK(tit::merge(true, T{2.0}, T{3.0}) == T{2.0});
  CHECK(tit::merge(false, T{2.0}) == T{0.0});
  CHECK(tit::merge(false, T{2.0}, T{3.0}) == T{3.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// TODO: add those!
TEST_CASE("tit::core::newton_raphson") {}
TEST_CASE("tit::core::bisection") {}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
