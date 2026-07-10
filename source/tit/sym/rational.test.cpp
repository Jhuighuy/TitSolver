/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <limits>

#include "tit/core/exception.hpp"
#include "tit/sym/rational.hpp"
#include "tit/testing/test.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sym::Rational normalization") {
  CHECK(Rational{} == Rational{0});
  CHECK(Rational{2, 4} == Rational{1, 2});
  CHECK(Rational{-2, 4} == Rational{-1, 2});
  CHECK(Rational{2, -4} == Rational{-1, 2});
  CHECK(Rational{-2, -4} == Rational{1, 2});
  CHECK(Rational{0, -17} == Rational{0});
  CHECK_THROWS_MSG((Rational{1, 0}), Exception, "denominator");
}

TEST_CASE("sym::Rational arithmetic") {
  CHECK(-Rational{2, 3} == Rational{-2, 3});
  CHECK(Rational{1, 2} + Rational{1, 3} == Rational{5, 6});
  CHECK(Rational{1, 2} - Rational{1, 3} == Rational{1, 6});
  CHECK(Rational{2, 3} * Rational{9, 10} == Rational{3, 5});
  CHECK(Rational{2, 3} / Rational{10, 9} == Rational{3, 5});
  CHECK_THROWS_MSG(Rational{1} / Rational{0}, Exception, "divide");
}

TEST_CASE("sym::Rational comparison") {
  CHECK(Rational{-1, 2} < Rational{0});
  CHECK(Rational{1, 3} < Rational{1, 2});
  CHECK(Rational{2, 4} == Rational{1, 2});
  CHECK(Rational{5, 3} > Rational{3, 2});
}

TEST_CASE("sym::Rational checked range") {
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  CHECK(Rational{min, min} == Rational{1});
  CHECK(Rational{max, max} == Rational{1});
  CHECK(Rational{max, 2} * Rational{2, max} == Rational{1});
  CHECK_THROWS_MSG(-Rational{min}, Exception, "overflow");
  CHECK_THROWS_MSG(Rational{max} + Rational{1}, Exception, "overflow");
  CHECK_THROWS_MSG(Rational{max} * Rational{2}, Exception, "overflow");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sym
