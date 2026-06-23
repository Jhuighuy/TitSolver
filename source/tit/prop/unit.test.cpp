/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/prop/unit.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Unit") {
  SUBCASE("success") {
    SUBCASE("symbol") {
      const auto unit = prop::Unit{"m/s"};
      CHECK(unit.symbol() == "m/s");
    }
    SUBCASE("unitless value accepted") {
      const auto unit = prop::Unit{"kg/m^3"};
      CHECK(unit.convert("7850.0") == 7850.0);
    }
    SUBCASE("matching explicit unit accepted") {
      const auto unit = prop::Unit{"m/s^2"};
      CHECK(unit.convert("9.81 m/s^2") == 9.81);
    }
    SUBCASE("surrounding whitespace accepted") {
      const auto unit = prop::Unit{"s"};
      CHECK(unit.convert("  1.0   s  ") == 1.0);
    }
  }
  SUBCASE("error") {
    SUBCASE("empty symbol") {
      CHECK_THROWS_MSG(prop::Unit{""}, tit::Exception, "must not be empty");
    }
    SUBCASE("empty value") {
      const auto unit = prop::Unit{"m"};
      CHECK_THROWS_MSG(unit.convert(""), tit::Exception, "empty string");
    }
    SUBCASE("non-number") {
      const auto unit = prop::Unit{"m"};
      CHECK_THROWS_MSG(unit.convert("bad m"),
                       tit::Exception,
                       "Expected real value");
    }
    SUBCASE("wrong explicit unit") {
      const auto unit = prop::Unit{"kg/m^3"};
      CHECK_THROWS_MSG(unit.convert("7.85 g/cm^3"),
                       tit::Exception,
                       "unit conversion is not implemented yet");
    }
    SUBCASE("extra text") {
      const auto unit = prop::Unit{"m"};
      CHECK_THROWS_MSG(unit.convert("1.0 m extra"),
                       tit::Exception,
                       "Unexpected text");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
