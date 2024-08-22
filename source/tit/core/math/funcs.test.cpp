/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <concepts>
#include <limits>

#include "tit/core/math.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define NUM_TYPES float, double

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sign", Num, NUM_TYPES) {
  CHECK(sign(Num{0}) == Num{0});
  CHECK(sign(+Num{2}) == +Num{1});
  CHECK(sign(-Num{2}) == -Num{1});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("pow", Num, NUM_TYPES) {
  CHECK(pow2(+Num{2}) == +Num{4});
  CHECK(pow2(-Num{2}) == +Num{4});
  CHECK(pow3(+Num{2}) == +Num{8});
  CHECK(pow3(-Num{2}) == -Num{8});
  CHECK(pow4(+Num{2}) == +Num{16});
  CHECK(pow4(-Num{2}) == +Num{16});
  CHECK(pow5(+Num{2}) == +Num{32});
  CHECK(pow5(-Num{2}) == -Num{32});
  CHECK(pow6(+Num{2}) == +Num{64});
  CHECK(pow6(-Num{2}) == +Num{64});
  CHECK(pow7(+Num{2}) == +Num{128});
  CHECK(pow7(-Num{2}) == -Num{128});
  CHECK(pow8(+Num{2}) == +Num{256});
  CHECK(pow8(-Num{2}) == +Num{256});
  CHECK(pow9(+Num{2}) == +Num{512});
  CHECK(pow9(-Num{2}) == -Num{512});
  if constexpr (std::floating_point<Num>) {
    CHECK(pow(+Num{2}, 10) == Num{1024});
    CHECK(pow(-Num{2}, 10) == Num{1024});
  }
}

TEST_CASE_TEMPLATE("horner", Num, NUM_TYPES) {
  CHECK(horner(Num{1}, {Num{1}}) == Num{1});
  CHECK(horner(Num{3}, {Num{1}, -Num{3}, Num{2}}) == Num{10});
  CHECK(horner(-Num{2}, {Num{4}, -Num{1}, Num{3}}) == Num{18});
  CHECK(horner(Num{3}, {Num{6}, Num{1}, -Num{4}, Num{1}}) == Num{0});
}

TEST_CASE_TEMPLATE("inverse", Num, NUM_TYPES) {
  CHECK(inverse(Num{2}) == 0.5);
  CHECK(inverse(Num{8}) == 0.125);
}

TEST_CASE_TEMPLATE("rsqrt", Num, NUM_TYPES) {
  CHECK(rsqrt(Num{0.25}) == Num{2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("avg", Num, NUM_TYPES) {
  CHECK(avg(Num{1}, Num{2}) == static_cast<Num>(1.5));
  CHECK(avg(Num{1}, Num{2}, Num{3}) == static_cast<Num>(2.0));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("tiny_number_v", Num, NUM_TYPES) {
  // Tiny number must be positive.
  STATIC_CHECK(tiny_number_v<Num> > 0.0);
  // Small number should be larger than machine epsilon.
  STATIC_CHECK(tiny_number_v<Num> >= std::numeric_limits<Num>::epsilon());
}

TEST_CASE_TEMPLATE("is_tiny", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(is_tiny(+Num{0.0}));
  CHECK(is_tiny(-Num{0.0}));
  CHECK_FALSE(is_tiny(+Num{1.0}));
  CHECK_FALSE(is_tiny(-Num{1.0}));
  // Check if comparisons with `tiny_number_v` work as expected.
  CHECK(is_tiny(+tiny_number_v<Num>));
  CHECK(is_tiny(-tiny_number_v<Num>));
  CHECK(is_tiny(+Num{0.1} * tiny_number_v<Num>));
  CHECK(is_tiny(-Num{0.1} * tiny_number_v<Num>));
  CHECK_FALSE(is_tiny(+Num{2.0} * tiny_number_v<Num>));
  CHECK_FALSE(is_tiny(-Num{2.0} * tiny_number_v<Num>));
}

TEST_CASE_TEMPLATE("approx_equal_to", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23}));
  CHECK(!approx_equal_to(Num{1.23}, Num{5.67}));
  // Check if comparisons with `tiny_number_v` work as expected.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + tiny_number_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - tiny_number_v<Num>, Num{1.23}));
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + Num{0.1} * tiny_number_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - Num{0.1} * tiny_number_v<Num>, Num{1.23}));
  CHECK(!approx_equal_to(Num{1.23}, Num{1.23} + Num{2.0} * tiny_number_v<Num>));
  CHECK(!approx_equal_to(Num{1.23} - Num{2.0} * tiny_number_v<Num>, Num{1.23}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
