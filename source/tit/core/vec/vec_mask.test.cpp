/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"

#include "tit/testing/strict.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define NUM_TYPES double, Strict<double>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask", Num, NUM_TYPES) {
  SUBCASE("zero initialization") {
    VecMask<Num, 2> v{};
    CHECK_FALSE(v[0]);
    CHECK_FALSE(v[1]);
  }
  SUBCASE("zero assignment") {
    VecMask<Num, 2> v{true, false};
    v = {};
    CHECK_FALSE(v[0]);
    CHECK_FALSE(v[1]);
  }
  SUBCASE("value initialization") {
    VecMask<Num, 2> const v(true);
    CHECK(v[0]);
    CHECK(v[1]);
  }
  SUBCASE("aggregate initialization") {
    VecMask<Num, 2> const v{true, false};
    CHECK(v[0]);
    CHECK_FALSE(v[1]);
  }
  SUBCASE("aggregate assignment") {
    VecMask<Num, 2> v;
    v = {true, false};
    CHECK(v[0]);
    CHECK_FALSE(v[1]);
  }
  SUBCASE("subscript") {
    VecMask<Num, 2> v;
    v[0] = true, v[1] = false;
    CHECK(v[0]);
    CHECK_FALSE(v[1]);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask::all_any", Num, NUM_TYPES) {
  SUBCASE("all") {
    auto const m = Vec{Num{1}, Num{2}} == Vec{Num{1}, Num{2}};
    CHECK(any(m));
    CHECK(all(m));
  }
  SUBCASE("any") {
    auto const m = Vec{Num{1}, Num{2}} == Vec{Num{1}, Num{3}};
    CHECK(any(m));
    CHECK_FALSE(all(m));
  }
  SUBCASE("none") {
    auto const m = Vec{Num{1}, Num{2}} == Vec{Num{3}, Num{4}};
    CHECK_FALSE(any(m));
    CHECK_FALSE(all(m));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
