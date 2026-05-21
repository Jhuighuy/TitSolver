/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Surface") {
  const geom::Surface<Vec<double, 2>> surf;

  CHECK(surf.num_verts() == 0);
  CHECK(surf.verts().empty());

  CHECK(surf.num_faces() == 0);
  CHECK(surf.faces().empty());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Surface::verts") {
  constexpr Vec v1{0.0, 0.0};
  constexpr Vec v2{1.0, 0.0};
  constexpr Vec v3{0.0, 1.0};

  geom::Surface<Vec<double, 2>> surf;

  surf.append_vert(v1);
  surf.append_vert(v2);
  surf.append_vert(v3);

  REQUIRE(surf.num_verts() == 3);

  CHECK(surf.vert(0) == v1);
  CHECK(surf.vert(1) == v2);
  CHECK(surf.vert(2) == v3);

  CHECK_RANGE_EQ(surf.verts(), {v1, v2, v3});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Surface::faces") {
  constexpr Vec v1{0.0, 0.0};
  constexpr Vec v2{1.0, 0.0};
  constexpr Vec v3{0.0, 1.0};

  geom::Surface<Vec<double, 2>> surf;

  surf.append_vert(v1);
  surf.append_vert(v2);
  surf.append_vert(v3);

  REQUIRE(surf.num_verts() == 3);

  surf.append_face({0, 1});
  surf.append_face({1, 2});
  surf.append_face({0, 2});

  REQUIRE(surf.num_faces() == 3);

  CHECK_RANGE_EQ(surf.face_verts(0), {0, 1});
  CHECK_RANGE_EQ(surf.face_verts(1), {1, 2});
  CHECK_RANGE_EQ(surf.face_verts(2), {0, 2});

  CHECK(surf.face(0).a() == v1);
  CHECK(surf.face(0).b() == v2);
  CHECK(surf.face(1).a() == v2);
  CHECK(surf.face(1).b() == v3);
  CHECK(surf.face(2).a() == v1);
  CHECK(surf.face(2).b() == v3);

  CHECK(surf.faces()[0].a() == v1);
  CHECK(surf.faces()[0].b() == v2);
  CHECK(surf.faces()[1].a() == v2);
  CHECK(surf.faces()[1].b() == v3);
  CHECK(surf.faces()[2].a() == v1);
  CHECK(surf.faces()[2].b() == v3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
