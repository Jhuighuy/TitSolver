/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str_utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_nocase_equal") {
  SUBCASE("char") {
    CHECK(str_nocase_equal('A', 'a'));
    CHECK_FALSE(str_nocase_equal('A', 'B'));
  }
  SUBCASE("string") {
    CHECK(str_nocase_equal("aBc", "AbC"));
    CHECK_FALSE(str_nocase_equal("aBc", "AbD"));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_to_int") {
  SUBCASE("valid") {
    CHECK(str_to_int("123") == 123);
    CHECK(str_to_int("-123") == -123);
  }
  SUBCASE("invalid") {
    CHECK_FALSE(str_to_int("123abc").has_value());
    CHECK_FALSE(str_to_int("not an integer").has_value());
  }
}

TEST_CASE("str_to_float") {
  SUBCASE("valid") {
    CHECK(str_to_float("123.456") == 123.456);
    CHECK(str_to_float("-123.456") == -123.456);
  }
  SUBCASE("invalid") {
    CHECK_FALSE(str_to_float("123.abc").has_value());
    CHECK_FALSE(str_to_float("not a float").has_value());
  }
}

TEST_CASE("str_to_bool") {
  SUBCASE("boolean values") {
    CHECK(str_to_bool("true").value_or(false));
    CHECK(str_to_bool("True").value_or(false));
    CHECK(str_to_bool("TRUE").value_or(false));
    CHECK_FALSE(str_to_bool("false").value_or(false));
    CHECK_FALSE(str_to_bool("False").value_or(false));
    CHECK_FALSE(str_to_bool("FALSE").value_or(false));
  }
  SUBCASE("integer values") {
    CHECK(str_to_bool("1").value_or(false));
    CHECK(str_to_bool("2").value_or(false));
    CHECK(str_to_bool("-1").value_or(false));
    CHECK_FALSE(str_to_bool("0").value_or(false));
  }
  SUBCASE("invalid") {
    CHECK_FALSE(str_to_bool("trueee").has_value());
    CHECK_FALSE(str_to_bool("not a bool").has_value());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
