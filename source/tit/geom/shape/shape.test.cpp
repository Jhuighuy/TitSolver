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

TEST_CASE("geom::Shape") {
  geom::Shape<Vec<double, 2>> shape{};
  CHECK(shape.num_verts() == 0);
  CHECK(shape.num_faces() == 0);
  CHECK(shape.verts().empty());

  shape.append_vert(Vec{0.0, 0.0});
  shape.append_vert(Vec{1.0, 0.0});
  shape.append_vert(Vec{0.0, 1.0});

  CHECK(shape.num_verts() == 3);
  CHECK(shape.vert(0) == Vec{0.0, 0.0});
  CHECK(shape.vert(1) == Vec{1.0, 0.0});
  CHECK(shape.vert(2) == Vec{0.0, 1.0});

  CHECK_RANGE_EQ(shape.verts(), {Vec{0.0, 0.0}, Vec{1.0, 0.0}, Vec{0.0, 1.0}});
}

TEST_CASE("geom::Shape::faces") {
  SUBCASE("2D triangle") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0});
    shape.append_vert(Vec{0.0, 1.0});
    shape.append_face({0, 1});
    shape.append_face({1, 2});
    shape.append_face({0, 2});

    CHECK(shape.num_faces() == 3);
    CHECK_RANGE_EQ(shape.face_verts(0), {0, 1});
    CHECK_RANGE_EQ(shape.face_verts(1), {1, 2});
    CHECK_RANGE_EQ(shape.face_verts(2), {0, 2});

    CHECK_RANGE_EQ(shape.face(0).verts(), {Vec{0.0, 0.0}, Vec{1.0, 0.0}});
    CHECK_RANGE_EQ(shape.face(1).verts(), {Vec{1.0, 0.0}, Vec{0.0, 1.0}});
    CHECK_RANGE_EQ(shape.face(2).verts(), {Vec{0.0, 0.0}, Vec{0.0, 1.0}});

    const auto faces = shape.faces();
    const auto iter = faces.begin();
    CHECK((*iter).verts()[0] == Vec{0.0, 0.0});
    CHECK((*iter).verts()[1] == Vec{1.0, 0.0});
  }
  SUBCASE("3D tetrahedron") {
    geom::Shape<Vec<double, 3>> shape{};
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0, 0.0});
    shape.append_vert(Vec{0.0, 1.0, 0.0});
    shape.append_vert(Vec{0.0, 0.0, 1.0});
    shape.append_face({0, 1, 2});
    shape.append_face({0, 1, 3});
    shape.append_face({0, 2, 3});
    shape.append_face({1, 2, 3});

    CHECK(shape.num_faces() == 4);
    CHECK_RANGE_EQ(shape.face_verts(0), {0, 1, 2});
    CHECK_RANGE_EQ(shape.face_verts(1), {0, 1, 3});
    CHECK_RANGE_EQ(shape.face_verts(2), {0, 2, 3});
    CHECK_RANGE_EQ(shape.face_verts(3), {1, 2, 3});

    CHECK_RANGE_EQ(shape.face(0).verts(),
                   {
                       Vec{0.0, 0.0, 0.0},
                       Vec{1.0, 0.0, 0.0},
                       Vec{0.0, 1.0, 0.0},
                   });

    const auto faces = shape.faces();
    const auto iter = faces.begin();
    CHECK((*iter).verts()[0] == Vec{0.0, 0.0, 0.0});
    CHECK((*iter).verts()[1] == Vec{1.0, 0.0, 0.0});
    CHECK((*iter).verts()[2] == Vec{0.0, 1.0, 0.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
