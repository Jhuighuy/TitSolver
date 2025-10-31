/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <limits>

#include "tit/core/math.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define FLOAT_TYPES float, double
#define UINT_TYPES unsigned int, unsigned long

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("tiny_v", Float, FLOAT_TYPES) {
  // Tiny number must not be less than machine epsilon.
  static_assert(tiny_v<Float> >= std::numeric_limits<Float>::epsilon());
}

TEST_CASE_TEMPLATE("is_tiny", Float, FLOAT_TYPES) {
  CHECK(is_tiny(+Float{0.0}));
  CHECK(is_tiny(-Float{0.0}));
  CHECK_FALSE(is_tiny(+Float{1.0}));
  CHECK_FALSE(is_tiny(-Float{1.0}));
  CHECK(is_tiny(+tiny_v<Float>));
  CHECK(is_tiny(-tiny_v<Float>));
  CHECK(is_tiny(+Float{0.1} * tiny_v<Float>));
  CHECK(is_tiny(-Float{0.1} * tiny_v<Float>));
  CHECK_FALSE(is_tiny(+Float{2.0} * tiny_v<Float>));
  CHECK_FALSE(is_tiny(-Float{2.0} * tiny_v<Float>));
}

TEST_CASE_TEMPLATE("approx_equal_to", Float, FLOAT_TYPES) {
  CHECK(approx_equal_to(Float{1.23}, Float{1.23}));
  CHECK_FALSE(approx_equal_to(Float{1.23}, Float{5.67}));
  CHECK(approx_equal_to(Float{1.23}, Float{1.23} + Float{0.1} * tiny_v<Float>));
  CHECK(approx_equal_to(Float{1.23} - Float{0.1} * tiny_v<Float>, Float{1.23}));
  CHECK_FALSE(approx_equal_to(Float{1.23}, //
                              Float{1.23} + Float{2.0} * tiny_v<Float>));
  CHECK_FALSE(approx_equal_to(Float{1.23} - Float{2.0} * tiny_v<Float>, //
                              Float{1.23}));
}

TEST_CASE_TEMPLATE("bitwise_equal", Float, FLOAT_TYPES) {
  // Check ordinary numbers.
  CHECK(bitwise_equal(Float{1.23}, Float{1.23}));
  CHECK_FALSE(bitwise_equal(Float{1.23}, Float{1.24}));
  // Zeros of different signs are not bitwise-equal.
  CHECK(Float{+0.0} == Float{-0.0});
  CHECK_FALSE(bitwise_equal(Float{+0.0}, Float{-0.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("pow", Float, FLOAT_TYPES) {
  CHECK(pow<0>(-Float{2}) == +Float{1});
  CHECK(pow(-Float{2}, 1) == -Float{2});
  CHECK(pow2(-Float{2}) == +Float{4});
  CHECK(pow<3>(-Float{2}) == -Float{8});
  CHECK(pow(-Float{2}, 4) == +Float{16});
  CHECK(pow<5>(-Float{2}) == -Float{32});
  CHECK(pow<6>(-Float{2}) == +Float{64});
  CHECK(pow<7>(-Float{2}) == -Float{128});
  CHECK(pow(-Float{2}, 8) == +Float{256});
  CHECK(pow<9>(-Float{2}) == -Float{512});
  CHECK(pow(-Float{2}, 10) == Float{1024});
}

TEST_CASE_TEMPLATE("horner", Float, FLOAT_TYPES) {
  CHECK(horner(Float{1}, {Float{1}}) == Float{1});
  CHECK(horner(Float{3}, {Float{1}, -Float{3}, Float{2}}) == Float{10});
  CHECK(horner(-Float{2}, {Float{4}, -Float{1}, Float{3}}) == Float{18});
  CHECK(horner(Float{3}, {Float{6}, Float{1}, -Float{4}, Float{1}}) ==
        Float{0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("inverse", Float, FLOAT_TYPES) {
  CHECK(inverse(Float{2}) == 0.5);
  CHECK(inverse(Float{8}) == 0.125);
}

TEST_CASE_TEMPLATE("divide_up", UInt, UINT_TYPES) {
  CHECK(divide_up(UInt{0}, UInt{10}) == UInt{0});
  CHECK(divide_up(UInt{3}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{7}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{10}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{11}, UInt{10}) == UInt{2});
  CHECK(divide_up(UInt{20}, UInt{10}) == UInt{2});
}

TEST_CASE_TEMPLATE("avg", Float, FLOAT_TYPES) {
  CHECK(avg(Float{1}, Float{2}) == static_cast<Float>(1.5));
  CHECK(avg(Float{1}, Float{2}, Float{3}) == static_cast<Float>(2.0));
}

TEST_CASE_TEMPLATE("havg", Float, FLOAT_TYPES) {
  CHECK(havg(Float{1.0}, Float{4.0}) == Float{1.6});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
