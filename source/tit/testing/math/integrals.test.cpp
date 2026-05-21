/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/testing/math/integrals.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::integrate<BBox>") {
  using std::numbers::pi;
  SUBCASE("sin") {
    SUBCASE("1D") {
      CHECK_APPROX_EQ(integrate([](auto x) { return sin(x[0]); },
                                geom::BBox{Vec{0.0}, Vec{pi}}),
                      2.0);
    }
    SUBCASE("2D") {
      CHECK_APPROX_EQ(integrate([](auto x) { return sin(x[0]) * sin(x[1]); },
                                geom::BBox{Vec{0.0, 0.0}, Vec{pi, pi}}),
                      4.0);
    }
    SUBCASE("3D") {
      CHECK_APPROX_EQ(
          integrate([](auto x) { return sin(x[0]) * sin(x[1]) * sin(x[2]); },
                    geom::BBox{Vec{0.0, 0.0, 0.0}, Vec{pi, pi, pi}}),
          8.0);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::integrate<BSphere>") {
  using std::numbers::pi;
  SUBCASE("constant") {
    SUBCASE("1D") {
      CHECK_APPROX_EQ(integrate([](auto /*x*/) { return 1.0; },
                                geom::BSphere{Vec{0.0}, pi}),
                      2.0 * pi);
    }
    SUBCASE("2D") {
      CHECK_APPROX_EQ(integrate([](auto /*x*/) { return 1.0; },
                                geom::BSphere{Vec{0.0, 0.0}, pi}),
                      pow<3>(pi));
    }
    SUBCASE("3D") {
      CHECK_APPROX_EQ(integrate([](auto) { return 1.0; },
                                geom::BSphere{Vec{0.0, 0.0, 0.0}, pi}),
                      4.0 / 3.0 * pow<4>(pi));
    }
  }
  SUBCASE("odd") {
    SUBCASE("1D") {
      CHECK_APPROX_EQ(
          integrate([](auto x) { return x[0]; }, geom::BSphere{Vec{0.0}, pi}),
          0.0);
    }
    SUBCASE("2D") {
      CHECK_APPROX_EQ(integrate([](auto x) { return x[0]; },
                                geom::BSphere{Vec{0.0, 0.0}, pi}),
                      0.0);
    }
    SUBCASE("3D") {
      CHECK_APPROX_EQ(integrate([](auto x) { return x[0]; },
                                geom::BSphere{Vec{0.0, 0.0, 0.0}, pi}),
                      0.0);
    }
  }
  SUBCASE("norm2") {
    SUBCASE("2D") {
      CHECK_APPROX_EQ(integrate([](auto x) { return norm2(x); },
                                geom::BSphere{Vec{0.0, 0.0}, 1.0}),
                      pi / 2.0);
    }
    SUBCASE("3D") {
      CHECK_APPROX_EQ(integrate([](const auto& x) { return norm2(x); },
                                geom::BSphere{Vec{0.0, 0.0, 0.0}, 1.0}),
                      4.0 * pi / 5.0);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
