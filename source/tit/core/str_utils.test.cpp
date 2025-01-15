/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
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
    CHECK_FALSE(str_nocase_equal("aBc", "AbD")); // codespell:ignore
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_to") {
  SUBCASE("int") {
    SUBCASE("valid") {
      CHECK(str_to<int>("123") == 123);
      CHECK(str_to<int>("-123") == -123);
    }
    SUBCASE("invalid") {
      CHECK_FALSE(str_to<int>("123abc").has_value());
      CHECK_FALSE(str_to<int>("not an integer").has_value());
    }
  }
  SUBCASE("bool") {
    SUBCASE("literals") {
      CHECK(str_to<bool>("true").value_or(false));
      CHECK(str_to<bool>("True").value_or(false));
      CHECK(str_to<bool>("TRUE").value_or(false));
      CHECK_FALSE(str_to<bool>("false").value_or(false));
      CHECK_FALSE(str_to<bool>("False").value_or(false));
      CHECK_FALSE(str_to<bool>("FALSE").value_or(false));
    }
    SUBCASE("integer values") {
      CHECK(str_to<bool>("1").value_or(false));
      CHECK(str_to<bool>("2").value_or(false));
      CHECK(str_to<bool>("-1").value_or(false));
      CHECK_FALSE(str_to<bool>("0").value_or(false));
    }
    SUBCASE("invalid") {
      CHECK_FALSE(str_to<bool>("trueee").has_value());
      CHECK_FALSE(str_to<bool>("not a bool").has_value());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
