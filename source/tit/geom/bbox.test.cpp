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
    geom::BBox<double, 2> const bbox{};
    CHECK(all(bbox.low() == Vec<double, 2>{}));
    CHECK(all(bbox.high() == Vec<double, 2>{}));
  }
  SUBCASE("point initialization") {
    Vec const point{1.0, 2.0};
    geom::BBox const bbox{point};
    CHECK(all(bbox.low() == point));
    CHECK(all(bbox.high() == point));
  }
  SUBCASE("point pair initialization") {
    geom::BBox const bbox{Vec{-1.0, 3.0}, Vec{0.0, 2.0}};
    CHECK(all(bbox.low() == Vec{-1.0, 2.0}));
    CHECK(all(bbox.high() == Vec{0.0, 3.0}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::center_and_extents") {
  geom::BBox const bbox{Vec{1.0, 1.0}, Vec{3.0, 3.0}};
  CHECK(all(bbox.center() == Vec{2.0, 2.0}));
  CHECK(all(bbox.extents() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::clamp") {
  geom::BBox const bbox{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  CHECK(all(bbox.clamp({-1.0, -1.0}) == bbox.low()));
  CHECK(all(bbox.clamp({3.0, 3.0}) == bbox.high()));
  CHECK(all(bbox.clamp({1.0, 1.0}) == Vec{1.0, 1.0}));
  CHECK(all(bbox.clamp({3.0, 1.0}) == Vec{2.0, 1.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::grow") {
  geom::BBox bbox{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  bbox.grow(0.5);
  CHECK(all(bbox.low() == Vec{-0.5, -0.5}));
  CHECK(all(bbox.high() == Vec{2.5, 2.5}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::expand") {
  geom::BBox bbox{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  bbox.expand(Vec{0.5, 0.5});
  CHECK(all(bbox.low() == Vec{0.0, 0.0}));
  CHECK(all(bbox.high() == Vec{2.0, 2.0}));
  bbox.expand(Vec{-0.5, -0.5});
  CHECK(all(bbox.low() == Vec{-0.5, -0.5}));
  CHECK(all(bbox.high() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::split") {
  geom::BBox const bbox{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  auto const [left, right] = bbox.split(0, 0.5);
  CHECK(all(left.low() == Vec{0.0, 0.0}));
  CHECK(all(left.high() == Vec{0.5, 2.0}));
  CHECK(all(right.low() == Vec{0.5, 0.0}));
  CHECK(all(right.high() == Vec{2.0, 2.0}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
