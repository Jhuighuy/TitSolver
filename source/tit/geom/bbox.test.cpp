/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox") {
  SUBCASE("zero initialization") {
    const geom::BBox<Vec<double, 2>> box{};
    CHECK(all(box.low() == Vec<double, 2>{}));
    CHECK(all(box.high() == Vec<double, 2>{}));
  }
  SUBCASE("point initialization") {
    const Vec point{1.0, 2.0};
    const geom::BBox box{point};
    CHECK(all(box.low() == point));
    CHECK(all(box.high() == point));
  }
  SUBCASE("point pair initialization") {
    const geom::BBox box{Vec{-1.0, 3.0}, Vec{0.0, 2.0}};
    CHECK(all(box.low() == Vec{-1.0, 2.0}));
    CHECK(all(box.high() == Vec{0.0, 3.0}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::center_and_extents") {
  const geom::BBox box{Vec{1.0, 1.0}, Vec{3.0, 3.0}};
  CHECK(all(box.center() == Vec{2.0, 2.0}));
  CHECK(all(box.extents() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::clamp") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  CHECK(all(box.clamp({-1.0, -1.0}) == box.low()));
  CHECK(all(box.clamp({3.0, 3.0}) == box.high()));
  CHECK(all(box.clamp({1.0, 1.0}) == Vec{1.0, 1.0}));
  CHECK(all(box.clamp({3.0, 1.0}) == Vec{2.0, 1.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::grow") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.grow(0.5);
  CHECK(all(box.low() == Vec{-0.5, -0.5}));
  CHECK(all(box.high() == Vec{2.5, 2.5}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::expand") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.expand(Vec{0.5, 0.5});
  CHECK(all(box.low() == Vec{0.0, 0.0}));
  CHECK(all(box.high() == Vec{2.0, 2.0}));
  box.expand(Vec{-0.5, -0.5});
  CHECK(all(box.low() == Vec{-0.5, -0.5}));
  CHECK(all(box.high() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::split") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  const auto [left, right] = box.split(0, 0.5);
  CHECK(all(left.low() == Vec{0.0, 0.0}));
  CHECK(all(left.high() == Vec{0.5, 2.0}));
  CHECK(all(right.low() == Vec{0.5, 0.0}));
  CHECK(all(right.high() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
