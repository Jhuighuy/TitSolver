/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/segment.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::verts") {
  constexpr Vec a{1.0, 2.0};
  constexpr Vec b{4.0, 6.0};

  const geom::Segment seg{a, b};

  CHECK_RANGE_EQ(seg.verts(), {a, b});

  CHECK(seg.a() == a);
  CHECK(seg.b() == b);
  CHECK(seg.ba() == b - a);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::box") {
  const geom::Segment seg{
      Vec{0.0, 1.0},
      Vec{1.0, 0.0},
  };
  CHECK(seg.box().low() == Vec{0.0, 0.0});
  CHECK(seg.box().high() == Vec{1.0, 1.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::center") {
  CHECK_APPROX_EQ(geom::Segment{Vec{0.0, 0.0}, Vec{3.0, 4.0}}.center(),
                  Vec{1.5, 2.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::wnormal") {
  CHECK_APPROX_EQ(geom::Segment{Vec{1.0, 2.0}, Vec{4.0, 6.0}}.wnormal(),
                  Vec{4.0, -3.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::normal") {
  CHECK_APPROX_EQ(geom::Segment{Vec{1.0, 2.0}, Vec{4.0, 6.0}}.normal(),
                  Vec{0.8, -0.6});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::length") {
  CHECK_APPROX_EQ(geom::Segment{Vec{0.0, 0.0}, Vec{3.0, 4.0}}.length(), 5.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::winding_number") {
  constexpr Vec a{1.0, 0.0};
  constexpr Vec b{1.0, 1.0};
  const Vec point{0.5, 0.5};
  SUBCASE("normal") {
    const geom::Segment segment{a, b};
    CHECK_APPROX_EQ(segment.winding_number(point), 0.25);
  }
  SUBCASE("flipped") {
    const geom::Segment segment{b, a};
    CHECK_APPROX_EQ(segment.winding_number(point), -0.25);
  }
  SUBCASE("translated") {
    constexpr Vec offset{3.0, 4.0};
    const geom::Segment segment{a + offset, b + offset};
    CHECK_APPROX_EQ(segment.winding_number(point + offset), 0.25);
  }
  SUBCASE("scaled") {
    constexpr auto scale = 2.0;
    const geom::Segment segment{scale * a, scale * b};
    CHECK_APPROX_EQ(segment.winding_number(scale * point), 0.25);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::clamp") {
  SUBCASE("normal") {
    const geom::Segment seg{
        Vec{0.0, 0.0},
        Vec{4.0, 4.0},
    };

    // Closest to vertex A.
    CHECK_APPROX_EQ(seg.clamp(Vec{0.0, 0.0}), Vec{0.0, 0.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{-1.0, -1.0}), Vec{0.0, 0.0});

    // Closest to vertex B.
    CHECK_APPROX_EQ(seg.clamp(Vec{4.0, 4.0}), Vec{4.0, 4.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{5.0, 5.0}), Vec{4.0, 4.0});

    // Closest point is inside the segment.
    CHECK_APPROX_EQ(seg.clamp(Vec{1.0, 1.0}), Vec{1.0, 1.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{2.0, 2.0}), Vec{2.0, 2.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{1.9, 2.1}), Vec{2.0, 2.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{2.1, 1.9}), Vec{2.0, 2.0});
  }
  SUBCASE("degenerate") {
    const geom::Segment seg{
        Vec{0.0, 0.0},
        Vec{0.0, 0.0},
    };

    // All points are closest to single vertex.
    CHECK_APPROX_EQ(seg.clamp(Vec{0.0, 0.0}), Vec{0.0, 0.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{1.0, 1.0}), Vec{0.0, 0.0});
    CHECK_APPROX_EQ(seg.clamp(Vec{-1.0, -1.0}), Vec{0.0, 0.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Segment::intersects") {
  const geom::Segment face{
      Vec{0.0, 0.0},
      Vec{2.0, 0.0},
  };
  CHECK(face.intersects(geom::BSphere{Vec{1.0, 0.0}, 0.5}));
  CHECK(face.intersects(geom::BSphere{Vec{0.0, 0.0}, 0.5}));
  CHECK(face.intersects(geom::BSphere{Vec{1.0, 0.5}, 0.5}));
  CHECK_FALSE(face.intersects(geom::BSphere{Vec{2.5, 0.5}, 0.5}));
  CHECK_FALSE(face.intersects(geom::BSphere{Vec{5.0, 5.0}, 0.5}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
