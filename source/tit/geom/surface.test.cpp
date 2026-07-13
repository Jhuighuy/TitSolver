/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/exception.hpp"
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

  constexpr std::array f1{0UZ, 1UZ};
  constexpr std::array f2{1UZ, 2UZ};
  constexpr std::array f3{2UZ, 0UZ};

  surf.append_face(f1);
  surf.append_face(f2);
  surf.append_face(f3);

  REQUIRE(surf.num_faces() == 3);

  CHECK(surf.face_verts(0) == f1);
  CHECK(surf.face_verts(1) == f2);
  CHECK(surf.face_verts(2) == f3);

  CHECK_RANGE_EQ(surf.face_verts(), {f1, f2, f3});

  CHECK(surf.face(0).a() == v1);
  CHECK(surf.face(0).b() == v2);
  CHECK(surf.face(1).a() == v2);
  CHECK(surf.face(1).b() == v3);
  CHECK(surf.face(2).a() == v3);
  CHECK(surf.face(2).b() == v1);

  CHECK(surf.faces()[0].a() == v1);
  CHECK(surf.faces()[0].b() == v2);
  CHECK(surf.faces()[1].a() == v2);
  CHECK(surf.faces()[1].b() == v3);
  CHECK(surf.faces()[2].a() == v3);
  CHECK(surf.faces()[2].b() == v1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::surface_cast") {
  geom::Surface<Vec<double, 2>> surf;
  surf.append_vert({0.0, 0.0});
  surf.append_vert({3.0, 0.0});
  surf.append_vert({0.0, 4.0});
  surf.append_face({0, 1});
  surf.append_face({1, 2});
  surf.append_face({2, 0});

  const auto int_surf = geom::surface_cast<Vec<int, 2>>(surf);

  CHECK_RANGE_EQ(int_surf.verts(), {{0, 0}, {3, 0}, {0, 4}});
  CHECK_RANGE_EQ(int_surf.face_verts(), surf.face_verts());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::surface_from_string") {
  constexpr auto tri_ply = R"PLY(ply
    format ascii 1.0
    element vertex 3
    property float x
    property float y
    property float z
    element face 3
    property list uchar int vertex_index
    end_header
    0 0 0
    3 0 0
    0 4 0
    2 0 1
    2 1 2
    2 2 0
  )PLY";
  constexpr auto tetra_ply = R"PLY(ply
    format ascii 1.0
    element vertex 4
    property float x
    property float y
    property float z
    element face 4
    property list uchar int vertex_index
    end_header
    0 0 0
    1 0 0
    0 1 0
    0 0 1
    3 0 1 2
    3 0 3 1
    3 0 2 3
    3 1 3 2
  )PLY";
  SUBCASE("success") {
    SUBCASE("2D") {
      const auto surf = geom::surface_from_string<Vec<double, 2>>(tri_ply);
      CHECK_RANGE_APPROX_EQ(surf.verts(),
                            {
                                {0.0, 0.0},
                                {3.0, 0.0},
                                {0.0, 4.0},
                            });
      CHECK_RANGE_EQ(surf.face_verts(),
                     {
                         {0, 1},
                         {1, 2},
                         {2, 0},
                     });
    }
    SUBCASE("3D") {
      const auto surf = geom::surface_from_string<Vec<double, 3>>(tetra_ply);
      CHECK_RANGE_APPROX_EQ(surf.verts(),
                            {
                                {0.0, 0.0, 0.0},
                                {1.0, 0.0, 0.0},
                                {0.0, 1.0, 0.0},
                                {0.0, 0.0, 1.0},
                            });
      CHECK_RANGE_EQ(surf.face_verts(),
                     {
                         {0, 1, 2},
                         {0, 3, 1},
                         {0, 2, 3},
                         {1, 3, 2},
                     });
    }
  }
  SUBCASE("failure") {
    SUBCASE("invalid format") {
      CHECK_THROWS_MSG((geom::surface_from_string<2>("invalid")),
                       Exception,
                       "No suitable reader found");
    }
    SUBCASE("wrong dimension") {
      CHECK_THROWS_MSG((geom::surface_from_string<2>(tetra_ply)),
                       Exception,
                       "is not 2D");
      CHECK_THROWS_MSG((geom::surface_from_string<3>(tri_ply)),
                       Exception,
                       "is not 3D");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::surface_dump_string") {
  SUBCASE("success") {
    SUBCASE("2D") {
      geom::Surface<Vec<double, 2>> surf;
      surf.append_vert({0.0, 0.0});
      surf.append_vert({3.0, 0.0});
      surf.append_vert({0.0, 4.0});
      surf.append_face({0, 1});
      surf.append_face({1, 2});
      surf.append_face({2, 0});

      const auto str = geom::surface_dump_string(surf, "ply");

      const auto surf2 = geom::surface_from_string<Vec<double, 2>>(str);
      CHECK_RANGE_APPROX_EQ(surf2.verts(), surf.verts());
      CHECK_RANGE_EQ(surf2.face_verts(), surf.face_verts());
    }
    SUBCASE("3D") {
      geom::Surface<Vec<double, 3>> surf;
      surf.append_vert({0.0, 0.0, 0.0});
      surf.append_vert({1.0, 0.0, 0.0});
      surf.append_vert({0.0, 1.0, 0.0});
      surf.append_vert({0.0, 0.0, 1.0});
      surf.append_face({0, 1, 2});
      surf.append_face({0, 3, 1});
      surf.append_face({0, 2, 3});
      surf.append_face({1, 3, 2});

      const auto str = geom::surface_dump_string(surf, "ply");

      const auto surf2 = geom::surface_from_string<Vec<double, 3>>(str);
      CHECK_RANGE_APPROX_EQ(surf2.verts(), surf.verts());
      CHECK_RANGE_EQ(surf2.face_verts(), surf.face_verts());
    }
  }
  SUBCASE("failure") {
    SUBCASE("invalid format") {
      const geom::Surface<Vec<double, 2>> surf;
      CHECK_THROWS_MSG((geom::surface_dump_string(surf, "invalid")),
                       Exception,
                       "Found no exporter");
    }
    SUBCASE("multiple files") {
      const geom::Surface<Vec<double, 2>> surf;
      CHECK_THROWS_MSG((geom::surface_dump_string(surf, "obj")),
                       Exception,
                       "produces multiple files");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
