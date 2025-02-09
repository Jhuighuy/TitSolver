/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <limits>

#include "tit/core/math.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define NUM_TYPES float, double

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("zero", Num, NUM_TYPES) {
  CHECK(zero(Num{2}) == Num{0});
}

TEST_CASE_TEMPLATE("tiny_v", Num, NUM_TYPES) {
  // Tiny number must not be less than machine epsilon.
  static_assert(tiny_v<Num> >= std::numeric_limits<Num>::epsilon());
}

TEST_CASE_TEMPLATE("is_tiny", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(is_tiny(+Num{0.0}));
  CHECK(is_tiny(-Num{0.0}));
  CHECK_FALSE(is_tiny(+Num{1.0}));
  CHECK_FALSE(is_tiny(-Num{1.0}));
  // Check if comparisons with `tiny_v` work as expected.
  CHECK(is_tiny(+tiny_v<Num>));
  CHECK(is_tiny(-tiny_v<Num>));
  CHECK(is_tiny(+Num{0.1} * tiny_v<Num>));
  CHECK(is_tiny(-Num{0.1} * tiny_v<Num>));
  CHECK_FALSE(is_tiny(+Num{2.0} * tiny_v<Num>));
  CHECK_FALSE(is_tiny(-Num{2.0} * tiny_v<Num>));
}

TEST_CASE_TEMPLATE("approx_equal_to", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23}));
  CHECK_FALSE(approx_equal_to(Num{1.23}, Num{5.67}));
  // Check if comparisons with `tiny_v` work as expected.
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + tiny_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - tiny_v<Num>, Num{1.23}));
  CHECK(approx_equal_to(Num{1.23}, Num{1.23} + Num{0.1} * tiny_v<Num>));
  CHECK(approx_equal_to(Num{1.23} - Num{0.1} * tiny_v<Num>, Num{1.23}));
  CHECK_FALSE(approx_equal_to(Num{1.23}, Num{1.23} + Num{2.0} * tiny_v<Num>));
  CHECK_FALSE(approx_equal_to(Num{1.23} - Num{2.0} * tiny_v<Num>, Num{1.23}));
}

TEST_CASE_TEMPLATE("bitwise_equal", Num, NUM_TYPES) {
  // Check ordinary numbers.
  CHECK(bitwise_equal(Num{1.23}, Num{1.23}));
  CHECK_FALSE(bitwise_equal(Num{1.23}, Num{1.24}));
  // NaNs are equal due to bitwise comparison.
  CHECK(bitwise_equal(std::numeric_limits<Num>::quiet_NaN(),
                      std::numeric_limits<Num>::quiet_NaN()));
  // Zeros with different signs are not equal due to bitwise comparison.
  CHECK_FALSE(bitwise_equal(Num{+0.0}, Num{-0.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("pow", Num, NUM_TYPES) {
  CHECK(pow2(-Num{2}) == +Num{4});
  CHECK(pow<3>(-Num{2}) == -Num{8});
  CHECK(pow<4>(-Num{2}) == +Num{16});
  CHECK(pow<5>(-Num{2}) == -Num{32});
  CHECK(pow<6>(-Num{2}) == +Num{64});
  CHECK(pow<7>(-Num{2}) == -Num{128});
  CHECK(pow<8>(-Num{2}) == +Num{256});
  CHECK(pow<9>(-Num{2}) == -Num{512});
  CHECK(pow(-Num{2}, 10) == Num{1024});
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("avg", Num, NUM_TYPES) {
  CHECK(avg(Num{1}, Num{2}) == static_cast<Num>(1.5));
  CHECK(avg(Num{1}, Num{2}, Num{3}) == static_cast<Num>(2.0));
}

TEST_CASE_TEMPLATE("havg", Num, NUM_TYPES) {
  CHECK(havg(Num{1.0}, Num{4.0}) == Num{1.6});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
