/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/vec.hpp"
#include "tit/testing/numbers/tagged.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// To test implementations with and without SIMD.
#define NUM_TYPES float, double, Tagged<double>

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

TEST_CASE_TEMPLATE("VecMask::any", Num, NUM_TYPES) {
  const auto run = []<size_t Dim>(std::index_sequence<Dim> /*dim*/) {
    FSUBCASE("Dim = {}", Dim) {
      SUBCASE("all false") {
        const VecMask<Num, Dim> m(false);
        CHECK_FALSE(any(m));
      }
      SUBCASE("all true") {
        const VecMask<Num, Dim> m(true);
        CHECK(any(m));
      }
      SUBCASE("true in the middle") {
        VecMask<Num, Dim> m(false);
        m[Dim / 2] = true;
        CHECK(any(m));
      }
      SUBCASE("true at the end") {
        VecMask<Num, Dim> m(false);
        m[Dim - 1] = true;
        CHECK(any(m));
      }
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

TEST_CASE_TEMPLATE("VecMask::all", Num, NUM_TYPES) {
  const auto run = []<size_t Dim>(std::index_sequence<Dim> /*dim*/) {
    FSUBCASE("Dim = {}", Dim) {
      SUBCASE("all false") {
        const VecMask<Num, Dim> m(false);
        CHECK_FALSE(m);
        CHECK_FALSE(all(m));
      }
      SUBCASE("all true") {
        const VecMask<Num, Dim> m(true);
        CHECK(m);
        CHECK(all(m));
      }
      SUBCASE("false in the middle") {
        VecMask<Num, Dim> m(true);
        m[Dim / 2] = false;
        CHECK_FALSE(m);
        CHECK_FALSE(all(m));
      }
      SUBCASE("false at the end") {
        VecMask<Num, Dim> m(true);
        m[Dim - 1] = false;
        CHECK_FALSE(m);
        CHECK_FALSE(all(m));
      }
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

TEST_CASE_TEMPLATE("VecMask::find_true", Num, NUM_TYPES) {
  const auto run = []<size_t Dim>(std::index_sequence<Dim> /*dim*/) {
    FSUBCASE("Dim = {}", Dim) {
      SUBCASE("all true") {
        const VecMask<Num, Dim> m(true);
        CHECK(find_true(m) == 0);
      }
      SUBCASE("all false") {
        const VecMask<Num, Dim> m(false);
        CHECK(find_true(m) == -1);
      }
      SUBCASE("true in the middle") {
        VecMask<Num, Dim> m(false);
        m[Dim / 2] = true;
        CHECK(find_true(m) == Dim / 2);
      }
      SUBCASE("true at the end") {
        VecMask<Num, Dim> m(false);
        m[Dim - 1] = true;
        CHECK(find_true(m) == Dim - 1);
      }
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
