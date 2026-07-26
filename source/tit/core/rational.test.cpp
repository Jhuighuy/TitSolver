/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <limits>

#include "tit/core/exception.hpp"
#include "tit/core/math.hpp"
#include "tit/core/rational.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Note: expressions given to the test macros carry an extra layer of
// parentheses, since the preprocessor does not treat braces as grouping.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Rational") {
  SUBCASE("default") {
    constexpr Rational a;
    STATIC_CHECK(a.num() == 0);
    STATIC_CHECK(a.den() == 1);
    STATIC_CHECK(a.is_zero());
    STATIC_CHECK(a.is_integer());
  }
  SUBCASE("integer") {
    constexpr Rational a{3};
    STATIC_CHECK(a.num() == 3);
    STATIC_CHECK(a.den() == 1);
    STATIC_CHECK_FALSE(a.is_zero());
    STATIC_CHECK(a.is_integer());
  }
  SUBCASE("reduced") {
    constexpr Rational a{6, 4};
    STATIC_CHECK(a.num() == 3);
    STATIC_CHECK(a.den() == 2);
    STATIC_CHECK_FALSE(a.is_integer());
  }
  SUBCASE("negative denominator") {
    // The sign must always be carried by the numerator.
    constexpr Rational a{3, -4};
    STATIC_CHECK(a.num() == -3);
    STATIC_CHECK(a.den() == 4);
  }
  SUBCASE("zero numerator") {
    constexpr Rational a{0, -7};
    STATIC_CHECK(a.num() == 0);
    STATIC_CHECK(a.den() == 1);
  }
  SUBCASE("zero denominator") {
    CHECK_THROWS_MSG((Rational{1, 0}),
                     Exception,
                     "denominator must not be zero");
  }
}

TEST_CASE("Rational::operator+") {
  STATIC_CHECK((Rational{1, 2} + Rational{1, 3} == Rational{5, 6}));
  STATIC_CHECK((Rational{1, 2} + Rational{1, 2} == 1));
  STATIC_CHECK((Rational{1, 2} + Rational{-1, 2} == 0));
  STATIC_CHECK((Rational{1, 2} + 1 == Rational{3, 2}));
  SUBCASE("compound") {
    Rational a{1, 4};
    a += Rational{1, 4};
    CHECK((a == Rational{1, 2}));
  }
}

TEST_CASE("Rational::operator-") {
  STATIC_CHECK((-Rational{1, 2} == Rational{-1, 2}));
  STATIC_CHECK((Rational{1, 2} - Rational{1, 3} == Rational{1, 6}));
  STATIC_CHECK((Rational{1, 3} - Rational{1, 3} == 0));
  STATIC_CHECK((1 - Rational{1, 4} == Rational{3, 4}));
}

TEST_CASE("Rational::operator*") {
  STATIC_CHECK((Rational{2, 3} * Rational{3, 4} == Rational{1, 2}));
  STATIC_CHECK((Rational{2, 3} * 0 == 0));
  STATIC_CHECK((Rational{-2, 3} * Rational{3, 2} == -1));
}

TEST_CASE("Rational::operator/") {
  STATIC_CHECK((Rational{2, 3} / Rational{4, 9} == Rational{3, 2}));
  STATIC_CHECK((Rational{1} / Rational{-2} == Rational{-1, 2}));
  CHECK_THROWS_MSG(Rational{1} / Rational{0}, Exception, "division by zero");
}

TEST_CASE("Rational::operator<=>") {
  STATIC_CHECK((Rational{1, 3} < Rational{1, 2}));
  STATIC_CHECK((Rational{-1, 2} < Rational{1, 3}));
  STATIC_CHECK((Rational{2, 4} == Rational{1, 2}));
  STATIC_CHECK((Rational{3, 2} > Rational{1}));
  STATIC_CHECK((Rational{5, 2} >= Rational{5, 2}));
}

TEST_CASE("Rational::overflow") {
  // The arithmetic is carried out in double width, so the operands whose
  // products would not fit into the storage type are still fine, as long as
  // the final result does fit.
  constexpr std::int64_t large = 3'037'000'493; // Just above `sqrt(2^63)`.
  STATIC_CHECK((Rational{large, 1} * Rational{1, large} == 1));
  // Results that do not fit are reported rather than silently wrapped.
  constexpr auto huge = std::numeric_limits<std::int64_t>::max();
  CHECK_THROWS_MSG(Rational{huge} + Rational{1}, Exception, "overflow");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// `pow` itself lives in `math.hpp`; check that it works for the rationals.
TEST_CASE("pow(Rational)") {
  STATIC_CHECK((pow(Rational{2, 3}, 0) == 1));
  STATIC_CHECK((pow(Rational{2, 3}, 1) == Rational{2, 3}));
  STATIC_CHECK((pow(Rational{2, 3}, 3) == Rational{8, 27}));
  STATIC_CHECK((pow(Rational{-1, 2}, 4) == Rational{1, 16}));
  STATIC_CHECK((pow(Rational{-1, 2}, 5) == Rational{-1, 32}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
