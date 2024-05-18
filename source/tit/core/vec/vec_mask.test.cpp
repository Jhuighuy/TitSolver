/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"

#include "tit/testing/strict.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Use custom number type in this test to avoid vectorization.
using Num = Strict<double>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("VecMask") {
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
    const VecMask<Num, 2> v(true);
    CHECK(v[0]);
    CHECK(v[1]);
  }
  SUBCASE("aggregate initialization") {
    const VecMask<Num, 2> v{true, false};
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

TEST_CASE("Vec::operator==") {
  CHECK(all(Vec{1.0_d, 2.0_d} == Vec{1.0_d, 2.0_d}));
  CHECK_FALSE(any(Vec{1.0_d, 2.0_d} == Vec{3.0_d, 4.0_d}));
}

TEST_CASE("Vec::operator!=") {
  CHECK(all(Vec{1.0_d, 2.0_d} != Vec{3.0_d, 4.0_d}));
  CHECK_FALSE(any(Vec{1.0_d, 2.0_d} != Vec{1.0_d, 2.0_d}));
}

TEST_CASE("Vec::operator<") {
  CHECK(all(Vec{1.0_d, 2.0_d} < Vec{3.0_d, 4.0_d}));
  CHECK_FALSE(any(Vec{3.0_d, 4.0_d} < Vec{1.0_d, 2.0_d}));
}

TEST_CASE("Vec::operator<=") {
  CHECK(all(Vec{1.0_d, 2.0_d} <= Vec{1.0_d, 2.0_d}));
  CHECK(all(Vec{1.0_d, 2.0_d} <= Vec{3.0_d, 4.0_d}));
  CHECK_FALSE(any(Vec{3.0_d, 4.0_d} <= Vec{1.0_d, 2.0_d}));
}

TEST_CASE("Vec::operator>") {
  CHECK(all(Vec{3.0_d, 4.0_d} > Vec{1.0_d, 2.0_d}));
  CHECK_FALSE(any(Vec{1.0_d, 2.0_d} > Vec{3.0_d, 4.0_d}));
}

TEST_CASE("Vec::operator>=") {
  CHECK(all(Vec{1.0_d, 2.0_d} >= Vec{1.0_d, 2.0_d}));
  CHECK(all(Vec{3.0_d, 4.0_d} >= Vec{1.0_d, 2.0_d}));
  CHECK_FALSE(any(Vec{1.0_d, 2.0_d} >= Vec{3.0_d, 4.0_d}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("VecMask::filter") {
  const auto m = Vec{1.0_d, 2.0_d} == Vec{3.0_d, 2.0_d};
  CHECK(all(filter(m, Vec{1.0_d, 2.0_d}) == Vec{0.0_d, 2.0_d}));
}

TEST_CASE("VecMask::select") {
  const auto m = Vec{1.0_d, 2.0_d} == Vec{3.0_d, 2.0_d};
  CHECK(all(select(m, Vec{1.0_d, 2.0_d}, Vec{3.0_d, 4.0_d}) == //
            Vec{3.0_d, 2.0_d}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("VecMask::all_any") {
  SUBCASE("all") {
    const auto m = Vec{1.0_d, 2.0_d} == Vec{1.0_d, 2.0_d};
    CHECK(any(m));
    CHECK(all(m));
  }
  SUBCASE("any") {
    const auto m = Vec{1.0_d, 2.0_d} == Vec{1.0_d, 3.0_d};
    CHECK(any(m));
    CHECK_FALSE(all(m));
  }
  SUBCASE("none") {
    const auto m = Vec{1.0_d, 2.0_d} == Vec{3.0_d, 4.0_d};
    CHECK_FALSE(any(m));
    CHECK_FALSE(all(m));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
