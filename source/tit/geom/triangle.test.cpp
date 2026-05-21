/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/triangle.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Triangle::verts") {
  constexpr Vec a{1.0, 2.0, 3.0};
  constexpr Vec b{4.0, 6.0, 7.0};
  constexpr Vec c{9.0, 12.0, 15.0};

  const geom::Triangle tri{a, b, c};

  CHECK_RANGE_EQ(tri.verts(), {a, b, c});

  CHECK(tri.a() == a);
  CHECK(tri.b() == b);
  CHECK(tri.c() == c);
  CHECK(tri.ba() == b - a);
  CHECK(tri.cb() == c - b);
  CHECK(tri.ca() == c - a);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Triangle::area") {
  const geom::Triangle tri{
      Vec{0.0, 0.0, 0.0},
      Vec{1.0, 0.0, 0.0},
      Vec{0.0, 1.0, 0.0},
  };
  CHECK(tri.area() == 0.5);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Face::normal") {
  const geom::Triangle tri{
      Vec{0.0, 0.0, 0.0},
      Vec{1.0, 0.0, 0.0},
      Vec{0.0, 1.0, 0.0},
  };
  CHECK(tri.normal() == Vec{0.0, 0.0, 1.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Triangle::box") {
  const geom::Triangle tri{
      Vec{1.0, 1.0, 1.0},
      Vec{4.0, 5.0, 6.0},
      Vec{0.0, 2.0, 3.0},
  };
  CHECK(tri.box().low() == Vec{0.0, 1.0, 1.0});
  CHECK(tri.box().high() == Vec{4.0, 5.0, 6.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Triangle::clamp") {
  const geom::Triangle tri{
      Vec{0.0, 0.0, 0.0},
      Vec{2.0, 0.0, 0.0},
      Vec{0.0, 2.0, 0.0},
  };

  // Region 1: Closest to vertex A.
  CHECK_APPROX_EQ(tri.clamp(Vec{-1.0, -1.0, 0.0}), Vec{0.0, 0.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{-0.5, -0.5, 5.0}), Vec{0.0, 0.0, 0.0});

  // Region 2: Closest to vertex B.
  CHECK_APPROX_EQ(tri.clamp(Vec{3.0, -1.0, 0.0}), Vec{2.0, 0.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{2.5, -0.5, -3.0}), Vec{2.0, 0.0, 0.0});

  // Region 3: Closest to vertex C.
  CHECK_APPROX_EQ(tri.clamp(Vec{-1.0, 3.0, 0.0}), Vec{0.0, 2.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{-0.5, 2.5, 2.0}), Vec{0.0, 2.0, 0.0});

  // Region 4: Closest to edge AB.
  CHECK_APPROX_EQ(tri.clamp(Vec{1.0, -1.0, 0.0}), Vec{1.0, 0.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{0.5, -2.0, -1.0}), Vec{0.5, 0.0, 0.0});

  // Region 5: Closest to edge AC.
  CHECK_APPROX_EQ(tri.clamp(Vec{-1.0, 1.0, 0.0}), Vec{0.0, 1.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{-2.0, 1.5, 4.0}), Vec{0.0, 1.5, 0.0});

  // Region 6: Closest to edge BC.
  CHECK_APPROX_EQ(tri.clamp(Vec{2.0, 2.0, 0.0}), Vec{1.0, 1.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{1.5, 1.5, -2.0}), Vec{1.0, 1.0, 0.0});

  // Region 7: Closest point is inside the triangle.
  CHECK_APPROX_EQ(tri.clamp(Vec{0.0, 0.0, 0.0}), Vec{0.0, 0.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{2.0, 0.0, 0.0}), Vec{2.0, 0.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{0.0, 2.0, 0.0}), Vec{0.0, 2.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{0.5, 0.5, 0.0}), Vec{0.5, 0.5, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{1.0, 1.0, 1.0}), Vec{1.0, 1.0, 0.0});
  CHECK_APPROX_EQ(tri.clamp(Vec{1.0, 1.0, -1.0}), Vec{1.0, 1.0, 0.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Triangle::intersects") {
  const geom::Triangle tri{
      Vec{0.0, 0.0, 0.0},
      Vec{2.0, 0.0, 0.0},
      Vec{0.0, 2.0, 0.0},
  };
  CHECK(tri.intersects(geom::BSphere{Vec{0.0, 0.0, 0.0}, 0.5}));
  CHECK(tri.intersects(geom::BSphere{Vec{0.5, 0.5, 0.2}, 0.3}));
  CHECK_FALSE(tri.intersects(geom::BSphere{Vec{10.0, 10.0, 10.0}, 0.5}));
  CHECK_FALSE(tri.intersects(geom::BSphere{Vec{0.5, 0.5, 5.0}, 0.5}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
