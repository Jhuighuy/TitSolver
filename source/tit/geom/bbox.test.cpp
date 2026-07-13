/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
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
    CHECK(box.low() == Vec<double, 2>{});
    CHECK(box.high() == Vec<double, 2>{});
  }
  SUBCASE("point initialization") {
    const Vec point{1.0, 2.0};
    const geom::BBox box{point};
    CHECK(box.low() == point);
    CHECK(box.high() == point);
  }
  SUBCASE("point pair initialization") {
    const geom::BBox box{Vec{-1.0, 3.0}, Vec{0.0, 2.0}};
    CHECK(box.low() == Vec{-1.0, 2.0});
    CHECK(box.high() == Vec{0.0, 3.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::center_and_extents") {
  const geom::BBox box{Vec{1.0, 1.0}, Vec{3.0, 3.0}};
  CHECK(box.center() == Vec{2.0, 2.0});
  CHECK(box.extents() == Vec{2.0, 2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::clamp") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  SUBCASE("point inside") {
    CHECK(box.clamp({1.0, 1.0}) == Vec{1.0, 1.0});
    CHECK(box.clamp({3.0, 1.0}) == Vec{2.0, 1.0});
  }
  SUBCASE("point outside") {
    CHECK(box.clamp({-1.0, -1.0}) == box.low());
    CHECK(box.clamp({3.0, 3.0}) == box.high());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::contains") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  SUBCASE("point inside") {
    CHECK(box.contains(Vec{1.0, 1.0}));
  }
  SUBCASE("point on boundary") {
    CHECK(box.contains(Vec{0.0, 0.0}));
    CHECK(box.contains(Vec{2.0, 2.0}));
    CHECK(box.contains(Vec{0.0, 1.0}));
  }
  SUBCASE("point outside") {
    CHECK_FALSE(box.contains(Vec{-1.0, 1.0}));
    CHECK_FALSE(box.contains(Vec{3.0, 3.0}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::intersects") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  SUBCASE("contained") {
    const geom::BBox other{Vec{0.5, 0.5}, Vec{1.5, 1.5}};
    CHECK(box.intersects(other));
    CHECK(other.intersects(box));
  }
  SUBCASE("overlapping") {
    const geom::BBox other{Vec{1.5, 1.5}, Vec{3.0, 3.0}};
    CHECK(box.intersects(other));
    CHECK(other.intersects(box));
  }
  SUBCASE("containing") {
    const geom::BBox other{Vec{-2.0, -2.0}, Vec{3.0, 3.0}};
    CHECK(box.intersects(other));
    CHECK(other.intersects(box));
  }
  SUBCASE("touching") {
    const geom::BBox other1{Vec{2.0, 0.5}, Vec{3.0, 1.5}};
    CHECK(box.intersects(other1));
    CHECK(other1.intersects(box));

    const geom::BBox other2{Vec{2.0, 2.0}, Vec{3.0, 3.0}};
    CHECK(box.intersects(other2));
    CHECK(other2.intersects(box));
  }
  SUBCASE("disjoint") {
    const geom::BBox other1{Vec{-2.0, -2.0}, Vec{-1.0, -1.0}};
    CHECK_FALSE(box.intersects(other1));
    CHECK_FALSE(other1.intersects(box));

    const geom::BBox other2{Vec{3.0, 3.0}, Vec{4.0, 4.0}};
    CHECK_FALSE(box.intersects(other2));
    CHECK_FALSE(other2.intersects(box));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::grow") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.grow(0.5);
  CHECK(box.low() == Vec{-0.5, -0.5});
  CHECK(box.high() == Vec{2.5, 2.5});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::shrink") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.shrink(0.5);
  CHECK(box.low() == Vec{0.5, 0.5});
  CHECK(box.high() == Vec{1.5, 1.5});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::expand") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.expand(Vec{0.5, 0.5});
  CHECK(box.low() == Vec{0.0, 0.0});
  CHECK(box.high() == Vec{2.0, 2.0});
  box.expand(Vec{-0.5, -0.5});
  CHECK(box.low() == Vec{-0.5, -0.5});
  CHECK(box.high() == Vec{2.0, 2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::intersect") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.intersect(geom::BBox{Vec{1.0, 1.0}, Vec{3.0, 3.0}});
  CHECK(box.low() == Vec{1.0, 1.0});
  CHECK(box.high() == Vec{2.0, 2.0});
}

TEST_CASE("geom::BBox::join") {
  geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  box.join(geom::BBox{Vec{1.0, 1.0}, Vec{3.0, 3.0}});
  CHECK(box.low() == Vec{0.0, 0.0});
  CHECK(box.high() == Vec{3.0, 3.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::BBox::split(plane)") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
  SUBCASE("default") { // same as direct.
    const auto [left, right] = box.split(0, 0.5);
    CHECK(left.low() == Vec{0.0, 0.0});
    CHECK(left.high() == Vec{0.5, 2.0});
    CHECK(right.low() == Vec{0.5, 0.0});
    CHECK(right.high() == Vec{2.0, 2.0});
  }
  SUBCASE("direct") {
    const auto [left, right] = box.split(0, 0.5, false);
    CHECK(left.low() == Vec{0.0, 0.0});
    CHECK(left.high() == Vec{0.5, 2.0});
    CHECK(right.low() == Vec{0.5, 0.0});
    CHECK(right.high() == Vec{2.0, 2.0});
  }
  SUBCASE("reverse") {
    const auto [left, right] = box.split(0, 0.5, true);
    CHECK(left.low() == Vec{0.5, 0.0});
    CHECK(left.high() == Vec{2.0, 2.0});
    CHECK(right.low() == Vec{0.0, 0.0});
    CHECK(right.high() == Vec{0.5, 2.0});
  }
}

TEST_CASE("geom::BBox::split(point)") {
  SUBCASE("1D") {
    const geom::BBox box{Vec{0.0}, Vec{2.0}};
    const auto parts = box.split(1.0);
    REQUIRE(parts.size() == 2);
    CHECK(parts[0].low() == Vec{0.0});
    CHECK(parts[1].low() == Vec{1.0});
    CHECK(parts[0].high() == Vec{1.0});
    CHECK(parts[1].high() == Vec{2.0});
  }
  SUBCASE("2D") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
    const auto parts = box.split(Vec{1.0, 1.0});
    REQUIRE(parts.size() == 4);
    CHECK(parts[0].low() == Vec{0.0, 0.0});
    CHECK(parts[1].low() == Vec{0.0, 1.0});
    CHECK(parts[2].low() == Vec{1.0, 0.0});
    CHECK(parts[3].low() == Vec{1.0, 1.0});
    CHECK(parts[0].high() == Vec{1.0, 1.0});
    CHECK(parts[1].high() == Vec{1.0, 2.0});
    CHECK(parts[2].high() == Vec{2.0, 1.0});
    CHECK(parts[3].high() == Vec{2.0, 2.0});
  }
  SUBCASE("3D") {
    const geom::BBox box{Vec{0.0, 0.0, 0.0}, Vec{2.0, 2.0, 2.0}};
    const auto parts = box.split(Vec{1.0, 1.0, 1.0});
    REQUIRE(parts.size() == 8);
    CHECK(parts[0].low() == Vec{0.0, 0.0, 0.0});
    CHECK(parts[1].low() == Vec{0.0, 0.0, 1.0});
    CHECK(parts[2].low() == Vec{0.0, 1.0, 0.0});
    CHECK(parts[3].low() == Vec{0.0, 1.0, 1.0});
    CHECK(parts[4].low() == Vec{1.0, 0.0, 0.0});
    CHECK(parts[5].low() == Vec{1.0, 0.0, 1.0});
    CHECK(parts[6].low() == Vec{1.0, 1.0, 0.0});
    CHECK(parts[7].low() == Vec{1.0, 1.0, 1.0});
    CHECK(parts[0].high() == Vec{1.0, 1.0, 1.0});
    CHECK(parts[1].high() == Vec{1.0, 1.0, 2.0});
    CHECK(parts[2].high() == Vec{1.0, 2.0, 1.0});
    CHECK(parts[3].high() == Vec{1.0, 2.0, 2.0});
    CHECK(parts[4].high() == Vec{2.0, 1.0, 1.0});
    CHECK(parts[5].high() == Vec{2.0, 1.0, 2.0});
    CHECK(parts[6].high() == Vec{2.0, 2.0, 1.0});
    CHECK(parts[7].high() == Vec{2.0, 2.0, 2.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
