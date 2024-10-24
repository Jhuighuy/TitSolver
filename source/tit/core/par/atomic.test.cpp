/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/par.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::fetch_and_add") {
  constexpr auto init = 10;
  constexpr auto delta = 20;
  auto val = init;
  // Ensure we are getting back the original value.
  CHECK(par::fetch_and_add(val, delta) == init);
  // Ensure that the value was updated correctly.
  CHECK(val == init + delta);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::fetch_and_add") {
  constexpr auto expected = 10;
  constexpr auto desired = 20;
  SUBCASE("success") {
    auto val = expected;
    CHECK(par::compare_exchange(val, expected, desired));
    CHECK(val == desired);
  }
  SUBCASE("failure") {
    constexpr auto unexpected = 30;
    auto val = unexpected;
    CHECK_FALSE(par::compare_exchange(val, expected, desired));
    CHECK(val == unexpected);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
