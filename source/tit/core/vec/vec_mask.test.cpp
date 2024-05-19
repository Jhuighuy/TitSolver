/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"

#include "tit/core/vec/vec_mask.hpp"
#include "tit/testing/strict.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// To test implementations with and without SIMD.
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
//
// Logical operations
//

TEST_CASE_TEMPLATE("VecMask::operator!", Num, NUM_TYPES) {
  CHECK(all(VecMask<Num, 2>{true, false} == !VecMask<Num, 2>{false, true}));
}

TEST_CASE_TEMPLATE("VecMask::operator&&", Num, NUM_TYPES) {
  CHECK(all((VecMask<Num, 4>{true, false, true, false} &&
             VecMask<Num, 4>{true, true, false, false}) ==
            VecMask<Num, 4>{true, false, false, false}));
}

TEST_CASE_TEMPLATE("VecMask::operator||", Num, NUM_TYPES) {
  CHECK(all((VecMask<Num, 4>{true, false, true, false} ||
             VecMask<Num, 4>{true, true, false, false}) ==
            VecMask<Num, 4>{true, true, true, false}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Comparison operations
//

TEST_CASE_TEMPLATE("VecMask::operator==", Num, NUM_TYPES) {
  CHECK(all((VecMask<Num, 4>{true, false, true, false} ==
             VecMask<Num, 4>{true, true, false, false}) ==
            VecMask<Num, 4>{true, false, false, true}));
}

TEST_CASE_TEMPLATE("VecMask::operator!=", Num, NUM_TYPES) {
  CHECK(all((VecMask<Num, 4>{true, false, true, false} !=
             VecMask<Num, 4>{true, true, false, false}) ==
            VecMask<Num, 4>{false, true, true, false}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Reduction
//

TEST_CASE_TEMPLATE("VecMask::all_and_any", Num, NUM_TYPES) {
  // To cover all the SIMD paths, 17 element vectors are needed.
  SUBCASE("all") {
    const VecMask<Num, 17> m(true);
    CHECK(any(m));
    CHECK(all(m));
  }
  SUBCASE("some") {
    SUBCASE("true in registers") {
      VecMask<Num, 17> m(false);
      m[9] = true;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
    SUBCASE("true in remainder") {
      VecMask<Num, 17> m(false);
      m[16] = true;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
    SUBCASE("false in remainder") {
      VecMask<Num, 17> m(true);
      m[16] = false;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
  }
  SUBCASE("none") {
    const VecMask<Num, 17> m(false);
    CHECK_FALSE(any(m));
    CHECK_FALSE(all(m));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
