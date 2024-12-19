/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/numbers/strict.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/vec.hpp"

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

TEST_CASE_TEMPLATE("VecMask::operator!", Num, NUM_TYPES) {
  CHECK(VecMask<Num, 2>{true, false} == !VecMask<Num, 2>{false, true});
}

TEST_CASE_TEMPLATE("VecMask::operator&&", Num, NUM_TYPES) {
  CHECK((VecMask<Num, 4>{true, false, true, false} &&
         VecMask<Num, 4>{true, true, false, false}) ==
        VecMask<Num, 4>{true, false, false, false});
}

TEST_CASE_TEMPLATE("VecMask::operator||", Num, NUM_TYPES) {
  CHECK((VecMask<Num, 4>{true, false, true, false} ||
         VecMask<Num, 4>{true, true, false, false}) ==
        VecMask<Num, 4>{true, true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask::operator==", Num, NUM_TYPES) {
  CHECK((VecMask<Num, 4>{true, false, true, false} ==
         VecMask<Num, 4>{true, true, false, false}) ==
        VecMask<Num, 4>{true, false, false, true});
}

TEST_CASE_TEMPLATE("VecMask::operator!=", Num, NUM_TYPES) {
  CHECK((VecMask<Num, 4>{true, false, true, false} !=
         VecMask<Num, 4>{true, true, false, false}) ==
        VecMask<Num, 4>{false, true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask::all_and_any", Num, NUM_TYPES) {
  constexpr auto Dim = 2 * simd::max_reg_size_v<double> + 1;
  SUBCASE("all") {
    const VecMask<Num, Dim> m(true);
    CHECK(any(m));
    CHECK(all(m));
  }
  SUBCASE("some") {
    SUBCASE("true in registers") {
      VecMask<Num, Dim> m(false);
      m[Dim / 2] = true;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
    SUBCASE("true in remainder") {
      VecMask<Num, Dim> m(false);
      m[Dim - 1] = true;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
    SUBCASE("false in remainder") {
      VecMask<Num, Dim> m(true);
      m[Dim - 1] = false;
      CHECK(any(m));
      CHECK_FALSE(all(m));
    }
  }
  SUBCASE("none") {
    const VecMask<Num, Dim> m(false);
    CHECK_FALSE(any(m));
    CHECK_FALSE(all(m));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask::count_true", Num, NUM_TYPES) {
  constexpr auto Dim = 2 * simd::max_reg_size_v<double> + 1;
  SUBCASE("all") {
    const VecMask<Num, Dim> m(true);
    CHECK(count_true(m) == Dim);
  }
  SUBCASE("some") {
    SUBCASE("true in registers") {
      VecMask<Num, Dim> m(false);
      m[Dim / 2] = true;
      CHECK(count_true(m) == 1);
    }
    SUBCASE("true in remainder") {
      VecMask<Num, Dim> m(false);
      m[Dim - 1] = true;
      CHECK(count_true(m) == 1);
    }
    SUBCASE("false in remainder") {
      VecMask<Num, Dim> m(true);
      m[Dim - 1] = false;
      CHECK(count_true(m) == Dim - 1);
    }
  }
  SUBCASE("none") {
    const VecMask<Num, 17> m(false);
    CHECK(count_true(m) == 0);
  }
}

TEST_CASE_TEMPLATE("VecMask::find_true", Num, NUM_TYPES) {
  constexpr auto Dim = 2 * simd::max_reg_size_v<double> + 1;
  SUBCASE("all") {
    const VecMask<Num, Dim> m(true);
    CHECK(find_true(m) == 0);
  }
  SUBCASE("some") {
    SUBCASE("true in registers") {
      VecMask<Num, Dim> m(false);
      m[Dim / 2] = true;
      CHECK(find_true(m) == Dim / 2);
    }
    SUBCASE("true in remainder") {
      VecMask<Num, Dim> m(false);
      m[Dim - 1] = true;
      CHECK(find_true(m) == Dim - 1);
    }
    SUBCASE("false in remainder") {
      VecMask<Num, Dim> m(true);
      m[Dim - 1] = false;
      CHECK(find_true(m) == 0);
    }
  }
  SUBCASE("none") {
    const VecMask<Num, Dim> m(false);
    CHECK(find_true(m) == -1);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
