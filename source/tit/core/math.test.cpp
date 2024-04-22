/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cmath>
#include <limits>
#include <numbers>

#include "tit/core/math.hpp"

#include "tit/testing/test.hpp"

namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using numeric_limits = std::numeric_limits<double>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("abs") {
  // Check signed integers.
  CHECK(tit::abs(0) == 0);
  CHECK(tit::abs(+2) == 2);
  CHECK(tit::abs(-2) == 2);
  // Check ordinary floating-point numbers.
  CHECK(tit::abs(0.0) == 0.0);
  CHECK(tit::abs(+2.0) == 2.0);
  CHECK(tit::abs(-2.0) == 2.0);
  // Check infinity.
  CHECK(tit::abs(+numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::abs(-numeric_limits::infinity()) == numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::abs(numeric_limits::quiet_NaN())));
}

TEST_CASE("plus") {
  // Check signed integers.
  CHECK(tit::plus(0) == 0);
  CHECK(tit::plus(+2) == 2);
  CHECK(tit::plus(-2) == 0);
  // Check ordinary floating-point numbers.
  CHECK(tit::plus(0.0) == 0.0);
  CHECK(tit::plus(+2.0) == 2.0);
  CHECK(tit::plus(-2.0) == 0.0);
  // Check infinity.
  CHECK(tit::plus(+numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::plus(-numeric_limits::infinity()) == 0.0);
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::plus(numeric_limits::quiet_NaN())));
}

TEST_CASE("minus") {
  // Check signed integers.
  CHECK(tit::minus(0) == 0);
  CHECK(tit::minus(+2) == 0);
  CHECK(tit::minus(-2) == -2);
  // Check ordinary floating-point numbers.
  CHECK(tit::minus(0.0) == 0.0);
  CHECK(tit::minus(+2.0) == 0.0);
  CHECK(tit::minus(-2.0) == -2.0);
  // Check infinity.
  CHECK(tit::minus(+numeric_limits::infinity()) == 0.0);
  CHECK(tit::minus(-numeric_limits::infinity()) == -numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::minus(numeric_limits::quiet_NaN())));
}

TEST_CASE("sign") {
  // Check signed integers.
  CHECK(tit::sign(0) == 0);
  CHECK(tit::sign(+2) == +1);
  CHECK(tit::sign(-2) == -1);
  // Check ordinary floating-point numbers.
  CHECK(tit::sign(0.0) == 0.0);
  CHECK(tit::sign(+2.0) == +1.0);
  CHECK(tit::sign(-2.0) == -1.0);
  // Check infinity.
  CHECK(tit::sign(+numeric_limits::infinity()) == 1.0);
  CHECK(tit::sign(-numeric_limits::infinity()) == -1.0);
  // Ensure NaN propagation.
  CHECK(std::isnan(numeric_limits::quiet_NaN()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("small_number_v") {
  // Small number must be positive.
  CHECK(tit::small_number_v<double> > 0.0);
  // Small number should be larger than machine epsilon.
  CHECK(tit::small_number_v<double> >= numeric_limits::epsilon());
}

TEST_CASE("is_zero") {
  // Check ordinary small floating-point numbers.
  CHECK(tit::is_zero(+0.0));
  CHECK(tit::is_zero(-0.0));
  CHECK(!tit::is_zero(+1.0));
  CHECK(!tit::is_zero(-1.0));
  // Check if comparisons with `small_number_v` work as expected.
  CHECK(tit::is_zero(+tit::small_number_v<double>));
  CHECK(tit::is_zero(-tit::small_number_v<double>));
  CHECK(tit::is_zero(+0.1 * tit::small_number_v<double>));
  CHECK(tit::is_zero(-0.1 * tit::small_number_v<double>));
  CHECK(!tit::is_zero(+2.0 * tit::small_number_v<double>));
  CHECK(!tit::is_zero(-2.0 * tit::small_number_v<double>));
  // Check infinity.
  CHECK(!tit::is_zero(+numeric_limits::infinity()));
  CHECK(!tit::is_zero(-numeric_limits::infinity()));
  // Check epsilon.
  CHECK(tit::is_zero(+numeric_limits::epsilon()));
  CHECK(tit::is_zero(-numeric_limits::epsilon()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("floor") {
  // Check ordinary non-negative numbers.
  CHECK(tit::floor(0.0) == 0.0);
  CHECK(tit::floor(1.1) == 1.0);
  CHECK(tit::floor(1.5) == 1.0);
  CHECK(tit::floor(1.9) == 1.0);
  // Check ordinary negative numbers.
  CHECK(tit::floor(-1.1) == -2.0);
  CHECK(tit::floor(-1.5) == -2.0);
  CHECK(tit::floor(-1.9) == -2.0);
  // Check infinity.
  CHECK(tit::floor(+numeric_limits::infinity()) == +numeric_limits::infinity());
  CHECK(tit::floor(-numeric_limits::infinity()) == -numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::floor(numeric_limits::quiet_NaN())));
}

TEST_CASE("round") {
  // Check ordinary non-negative numbers.
  CHECK(tit::round(0.0) == 0.0);
  CHECK(tit::round(1.1) == 1.0);
  CHECK(tit::round(1.5) == 2.0);
  CHECK(tit::round(1.9) == 2.0);
  // Check ordinary negative numbers.
  CHECK(tit::round(-1.1) == -1.0);
  CHECK(tit::round(-1.5) == -2.0);
  CHECK(tit::round(-1.9) == -2.0);
  // Check infinity.
  CHECK(tit::round(+numeric_limits::infinity()) == +numeric_limits::infinity());
  CHECK(tit::round(-numeric_limits::infinity()) == -numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::round(numeric_limits::quiet_NaN())));
}

TEST_CASE("ceil") {
  // Check ordinary non-negative numbers.
  CHECK(tit::ceil(0.0) == 0.0);
  CHECK(tit::ceil(1.1) == 2.0);
  CHECK(tit::ceil(1.5) == 2.0);
  CHECK(tit::ceil(1.9) == 2.0);
  // Check ordinary negative numbers.
  CHECK(tit::ceil(-1.1) == -1.0);
  CHECK(tit::ceil(-1.5) == -1.0);
  CHECK(tit::ceil(-1.9) == -1.0);
  // Check infinity.
  CHECK(tit::ceil(+numeric_limits::infinity()) == +numeric_limits::infinity());
  CHECK(tit::ceil(-numeric_limits::infinity()) == -numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::ceil(numeric_limits::quiet_NaN())));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("inverse") {
  // Check ordinary numbers.
  CHECK(tit::inverse(2) == 0.5);
  CHECK(tit::inverse(10) == 0.1);
  CHECK(tit::inverse(10.0) == 0.1);
  // Check infinity.
  CHECK(tit::inverse(numeric_limits::infinity()) == 0.0);
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::inverse(numeric_limits::quiet_NaN())));
}

TEST_CASE("divide") {
  // Check ordinary divisors.
  CHECK(tit::divide(1, 2) == 0.5);
  CHECK(tit::divide(1, 10) == 0.1);
  CHECK(tit::divide(1, 10.0) == 0.1);
  // Check infinity.
  CHECK(tit::divide(1.0, numeric_limits::infinity()) == 0.0);
  CHECK(std::isnan(tit::divide(numeric_limits::infinity(), //
                               numeric_limits::infinity())));
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::divide(numeric_limits::quiet_NaN(), 1.0)));
  CHECK(std::isnan(tit::divide(1.0, numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::divide(numeric_limits::quiet_NaN(), //
                               numeric_limits::quiet_NaN())));
}

TEST_CASE("safe_inverse") {
  // Check ordinary non-"small" numbers.
  CHECK(tit::safe_inverse(2.0) == 0.5);
  CHECK(tit::safe_inverse(10.0) == 0.1);
  // Check small numbers.
  CHECK(tit::safe_inverse(0.0) == 0.0);
  CHECK(tit::safe_inverse(tit::small_number_v<double>) == 0.0);
  CHECK(tit::safe_inverse(0.1 * tit::small_number_v<double>) == 0.0);
  CHECK(tit::safe_inverse(2.0 * tit::small_number_v<double>) != 0.0);
  // Check infinity.
  CHECK(tit::safe_inverse(numeric_limits::infinity()) == 0.0);
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::safe_inverse(numeric_limits::quiet_NaN())));
}

TEST_CASE("safe_divide") {
  // Check ordinary non-"small" divisors.
  CHECK(tit::safe_divide(1, 2.0) == 0.5);
  CHECK(tit::safe_divide(1.0, 10.0) == 0.1);
  // Check small divisors.
  CHECK(tit::safe_divide(1.0, 0.0) == 0.0);
  CHECK(tit::safe_divide(1.0, tit::small_number_v<double>) == 0.0);
  CHECK(tit::safe_divide(1.0, 0.1 * tit::small_number_v<double>) == 0.0);
  CHECK(tit::safe_divide(1.0, 2.0 * tit::small_number_v<double>) != 0.0);
  // Check infinity.
  CHECK(tit::safe_divide(1.0, numeric_limits::infinity()) == 0.0);
  CHECK(std::isnan(tit::safe_divide(numeric_limits::infinity(),
                                    numeric_limits::infinity())));
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::safe_divide(numeric_limits::quiet_NaN(), 1.0)));
  CHECK(std::isnan(tit::safe_divide(1.0, numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::safe_divide(numeric_limits::quiet_NaN(),
                                    numeric_limits::quiet_NaN())));
}

TEST_CASE("ceil_divide") {
  CHECK(tit::ceil_divide(0U, 10U) == 0U);
  CHECK(tit::ceil_divide(3U, 10U) == 1U);
  CHECK(tit::ceil_divide(7U, 10U) == 1U);
  CHECK(tit::ceil_divide(10U, 10U) == 1U);
  CHECK(tit::ceil_divide(11U, 10U) == 2U);
  CHECK(tit::ceil_divide(20U, 10U) == 2U);
}

TEST_CASE("align") {
  CHECK(tit::align(0U, 10U) == 0U);
  CHECK(tit::align(3U, 10U) == 10U);
  CHECK(tit::align(7U, 10U) == 10U);
  CHECK(tit::align(10U, 10U) == 10U);
  CHECK(tit::align(11U, 10U) == 20U);
  CHECK(tit::align(20U, 10U) == 20U);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("pow") {
  // Check ordinary numbers.
  CHECK(tit::pow2(+2.0) == +4.0);
  CHECK(tit::pow2(-2.0) == +4.0);
  CHECK(tit::pow3(+2.0) == +8.0);
  CHECK(tit::pow3(-2.0) == -8.0);
  CHECK(tit::pow4(+2.0) == +16.0);
  CHECK(tit::pow4(-2.0) == +16.0);
  CHECK(tit::pow5(+2.0) == +32.0);
  CHECK(tit::pow5(-2.0) == -32.0);
  CHECK(tit::pow6(+2.0) == +64.0);
  CHECK(tit::pow6(-2.0) == +64.0);
  CHECK(tit::pow7(+2.0) == +128.0);
  CHECK(tit::pow7(-2.0) == -128.0);
  CHECK(tit::pow8(+2.0) == +256.0);
  CHECK(tit::pow8(-2.0) == +256.0);
  CHECK(tit::pow9(+2.0) == +512.0);
  CHECK(tit::pow9(-2.0) == -512.0);
  CHECK(tit::pow(2.0, 10) == 1024.0);
  CHECK(tit::pow(2.0, 10.0) == 1024.0);
  // Check infinity.
  CHECK(tit::pow2(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow3(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow4(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow5(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow6(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow7(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow8(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow9(numeric_limits::infinity()) == numeric_limits::infinity());
  CHECK(tit::pow(numeric_limits::infinity(), 10) == numeric_limits::infinity());
  CHECK(tit::pow(numeric_limits::infinity(), 10.0) ==
        numeric_limits::infinity());
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::pow2(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow3(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow4(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow5(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow6(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow7(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow8(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow9(numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::pow(numeric_limits::quiet_NaN(), 10)));
  CHECK(std::isnan(tit::pow(numeric_limits::quiet_NaN(), 10.0)));
}

TEST_CASE("sqrt") {
  // `tit::sqrt` is just a wrapper for `std::sqrt`, no need to test it much.
  CHECK(tit::sqrt(4.0) == 2.0);
}

TEST_CASE("cbrt") {
  // `tit::cbrt` is just a wrapper for `std::cbrt`, no need to test it much.
  CHECK(tit::cbrt(+8.0) == +2.0);
  CHECK(tit::cbrt(-8.0) == -2.0);
}

TEST_CASE("hypot") {
  // `tit::hypot` is just a wrapper for `std::hypot`, no need to test it much.
  CHECK(tit::hypot(3.0, 4.0) == 5.0);
  CHECK(tit::hypot(2.0, 6.0, 9.0) == 11.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("exp") {
  // `tit::exp` is just a wrapper for `std::exp`, no need to test it much.
  CHECK(tit::exp(1.0) == std::numbers::e_v<double>);
}

TEST_CASE("exp2") {
  // `tit::exp2` with floating-point arguments is just a wrapper for
  // `std::exp2`, no need to test it much.
  CHECK(tit::exp2(1.0) == 2.0);
  // Check unsigned integers.
  CHECK(tit::exp2(0U) == 1U);
  CHECK(tit::exp2(1U) == 2U);
  CHECK(tit::exp2(9U) == 512U);
}

TEST_CASE("log") {
  // `tit::log` is just a wrapper for `std::log`, no need to test it much.
  CHECK(tit::log(std::numbers::e_v<double>) == 1.0);
}

TEST_CASE("log2") {
  // `tit::log2` with floating-point arguments is just a wrapper for
  // `std::log2`, no need to test it much.
  CHECK(tit::log2(2.0) == 1.0);
  // Check unsigned integers.
  CHECK(tit::log2(1U) == 0U);
  CHECK(tit::log2(2U) == 1U);
  CHECK(tit::log2(512U) == 9U);
}

TEST_CASE("is_power_of_two") {
  CHECK(tit::is_power_of_two(0U));
  CHECK(tit::is_power_of_two(1U));
  CHECK(tit::is_power_of_two(512U));
  CHECK(!tit::is_power_of_two(255U));
  CHECK(!tit::is_power_of_two(513U));
}

TEST_CASE("align_to_power_of_two") {
  CHECK(tit::align_to_power_of_two(0U) == 0U);
  CHECK(tit::align_to_power_of_two(1U) == 1U);
  CHECK(tit::align_to_power_of_two(2U) == 2U);
  CHECK(tit::align_to_power_of_two(3U) == 4U);
  CHECK(tit::align_to_power_of_two(127U) == 128U);
  CHECK(tit::align_to_power_of_two(128U) == 128U);
  CHECK(tit::align_to_power_of_two(129U) == 256U);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("avg") {
  // Check ordinary numbers.
  CHECK(tit::avg(1, 2) == 1.5);
  CHECK(tit::avg(1, 2, 3) == 2.0);
  // Check infinity.
  CHECK(tit::avg(1, numeric_limits::infinity(), 3) ==
        numeric_limits::infinity());
  CHECK(std::isnan(tit::avg(+numeric_limits::quiet_NaN(), //
                            -numeric_limits::quiet_NaN())));
  // Ensure NaN propagation.
  CHECK(std::isnan(tit::avg(1, numeric_limits::quiet_NaN(), 3)));
}

TEST_CASE("gavg") {
  // Check ordinary numbers.
  CHECK(tit::havg(1.0, 4.0) == 1.6);
  // Check infinity.
  CHECK(tit::havg(1.0, numeric_limits::infinity()) == 2.0);
  CHECK(tit::havg(numeric_limits::infinity(), //
                  numeric_limits::infinity(), 3.0) == 9.0);
  // No NaN propagation -- input must be positive by contract.
}

TEST_CASE("gavg") {
  // Check ordinary numbers.
  CHECK(tit::gavg(1.0, 4.0) == 2.0);
  // Check infinity.
  CHECK(tit::gavg(1.0, numeric_limits::infinity(), 3.0) ==
        numeric_limits::infinity());
  CHECK(tit::gavg(numeric_limits::infinity(), //
                  numeric_limits::infinity(),
                  3.0) == numeric_limits::infinity());
  // No NaN propagation -- input must be positive by contract.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("merge") {
  // Check ordinary numbers.
  CHECK(tit::merge(true, 2.0) == 2.0);
  CHECK(tit::merge(true, 2.0, 3.0) == 2.0);
  CHECK(tit::merge(false, 2.0) == 0.0);
  CHECK(tit::merge(false, 2.0, 3.0) == 3.0);
  // Check infinity.
  CHECK(tit::merge(true, numeric_limits::infinity()) ==
        numeric_limits::infinity());
  CHECK(tit::merge(true, numeric_limits::infinity(), 3.0) ==
        numeric_limits::infinity());
  CHECK(tit::merge(false, numeric_limits::infinity()) == 0.0);
  CHECK(tit::merge(false, numeric_limits::infinity(), 3.0) == 3.0);
  // Check NaN.
  CHECK(std::isnan(tit::merge(true, numeric_limits::quiet_NaN())));
  CHECK(std::isnan(tit::merge(true, numeric_limits::quiet_NaN(), 3.0)));
  CHECK(tit::merge(false, numeric_limits::quiet_NaN()) == 0.0);
  CHECK(tit::merge(false, numeric_limits::quiet_NaN(), 3.0) == 3.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
