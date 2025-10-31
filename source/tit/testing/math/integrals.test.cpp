/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/testing/math/integrals.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("integrate") {
  using std::numbers::pi;
  SUBCASE("sin") {
    SUBCASE("1D") {
      using Box1D = geom::BBox<Vec<double, 1>>;
      CHECK_APPROX_EQ(
          integrate([](auto x) { return sin(x[0]); }, Box1D{0.0, pi}),
          2.0);
    }
    SUBCASE("2D") {
      using Box2D = geom::BBox<Vec<double, 2>>;
      CHECK_APPROX_EQ( //
          integrate([](auto x) { return sin(x[0]) * sin(x[1]); },
                    Box2D{{0.0, 0.0}, {pi, pi}}),
          4.0);
    }
    SUBCASE("3D") {
      using Box3D = geom::BBox<Vec<double, 3>>;
      CHECK_APPROX_EQ(
          integrate([](auto x) { return sin(x[0]) * sin(x[1]) * sin(x[2]); },
                    Box3D{{0.0, 0.0, 0.0}, {pi, pi, pi}}),
          8.0);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("integrate_cr") {
  CHECK_APPROX_EQ(integrate_cr([](const auto& x) { return norm2(x); }, 1.0),
                  std::numbers::pi / 2.0);
}

TEST_CASE("integrate_sp") {
  CHECK_APPROX_EQ(integrate_sp([](const auto& x) { return norm2(x); }, 1.0),
                  4.0 * std::numbers::pi / 5.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
