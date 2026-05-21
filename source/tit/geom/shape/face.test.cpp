/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/vec.hpp"
#include "tit/geom/shape.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Face") {
  SUBCASE("2D") {
    const geom::Face face{std::array{
        Vec{1.0, 2.0},
        Vec{4.0, 6.0},
    }};
    CHECK(face.verts().size() == 2);
    CHECK(face.verts()[0] == Vec{1.0, 2.0});
    CHECK(face.verts()[1] == Vec{4.0, 6.0});
  }
  SUBCASE("3D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0, 0.0},
        Vec{1.0, 0.0, 0.0},
        Vec{0.0, 1.0, 0.0},
    }};
    CHECK(face.verts()[0] == Vec{0.0, 0.0, 0.0});
    CHECK(face.verts()[1] == Vec{1.0, 0.0, 0.0});
    CHECK(face.verts()[2] == Vec{0.0, 1.0, 0.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Face::box") {
  SUBCASE("2D") {
    const geom::Face face{std::array{
        Vec{1.0, 2.0},
        Vec{4.0, 6.0},
    }};
    CHECK(face.box().low() == Vec{1.0, 2.0});
    CHECK(face.box().high() == Vec{4.0, 6.0});
  }
  SUBCASE("3D") {
    const geom::Face face{std::array{
        Vec{1.0, 1.0, 1.0},
        Vec{4.0, 5.0, 6.0},
        Vec{0.0, 2.0, 3.0},
    }};
    CHECK(face.box().low() == Vec{0.0, 1.0, 1.0});
    CHECK(face.box().high() == Vec{4.0, 5.0, 6.0});
  }
}

TEST_CASE("geom::Face::area") {
  SUBCASE("2D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0},
        Vec{3.0, 4.0},
    }};
    CHECK(face.area() == 5.0);
  }
  SUBCASE("3D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0, 0.0},
        Vec{1.0, 0.0, 0.0},
        Vec{0.0, 1.0, 0.0},
    }};

    CHECK(face.area() == 0.5);
  }
}

TEST_CASE("geom::Face::normal") {
  SUBCASE("2D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0},
        Vec{3.0, 4.0},
    }};
    CHECK_APPROX_EQ(face.normal(), Vec{0.8, -0.6});
  }
  SUBCASE("3D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0, 0.0},
        Vec{1.0, 0.0, 0.0},
        Vec{0.0, 1.0, 0.0},
    }};
    CHECK(face.normal() == Vec{0.0, 0.0, 1.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Face::intersects_sphere") {
  SUBCASE("2D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0},
        Vec{2.0, 0.0},
    }};
    CHECK(face.intersects_sphere(Vec{1.0, 0.0}, 0.5));
    CHECK(face.intersects_sphere(Vec{0.0, 0.0}, 0.5));
    CHECK_FALSE(face.intersects_sphere(Vec{5.0, 5.0}, 0.5));
  }
  SUBCASE("3D") {
    const geom::Face face{std::array{
        Vec{0.0, 0.0, 0.0},
        Vec{2.0, 0.0, 0.0},
        Vec{0.0, 2.0, 0.0},
    }};
    CHECK(face.intersects_sphere(Vec{0.0, 0.0, 0.0}, 0.5));
    CHECK(face.intersects_sphere(Vec{0.5, 0.5, 0.2}, 0.3));
    CHECK_FALSE(face.intersects_sphere(Vec{10.0, 10.0, 10.0}, 0.5));
    CHECK_FALSE(face.intersects_sphere(Vec{0.5, 0.5, 5.0}, 0.5));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
