/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BSphere") {
  SUBCASE("zero initialization") {
    const geom::BSphere<Vec<double, 2>> sphere{};
    CHECK(sphere.center() == Vec<double, 2>{});
    CHECK(sphere.radius() == 0.0);
  }
  SUBCASE("center initialization") {
    const Vec point{1.0, 2.0};
    const geom::BSphere sphere{point};
    CHECK(sphere.center() == point);
    CHECK(sphere.radius() == 0.0);
  }
  SUBCASE("center and radius initialization") {
    const geom::BSphere sphere{Vec{1.0, 2.0}, 3.0};
    CHECK(sphere.center() == Vec{1.0, 2.0});
    CHECK(sphere.radius() == 3.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BSphere::box") {
  const geom::BSphere sphere{Vec{1.0, 2.0}, 3.0};
  CHECK(sphere.box().low() == Vec{-2.0, -1.0});
  CHECK(sphere.box().high() == Vec{4.0, 5.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BSphere::contains") {
  const geom::BSphere sphere{Vec{1.0, 2.0}, 3.0};
  SUBCASE("point inside") {
    CHECK(sphere.contains(Vec{1.0, 2.0}));
    CHECK(sphere.contains(Vec{1.5, 2.5}));
  }
  SUBCASE("point on boundary") {
    CHECK(sphere.contains(Vec{4.0, 2.0}));
    CHECK(sphere.contains(Vec{-2.0, 2.0}));
  }
  SUBCASE("point outside") {
    CHECK_FALSE(sphere.contains(Vec{-3.0, 2.0}));
    CHECK_FALSE(sphere.contains(Vec{5.0, 5.0}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
