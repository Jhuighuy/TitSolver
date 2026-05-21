/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>

#include "tit/core/vec.hpp"
#include "tit/geom/shape/shape.hpp"
#include "tit/geom/shape/tessellate.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::tessellate") {
  SUBCASE("2D no-op") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{3.0, 4.0});
    shape.append_face({0, 1});

    geom::tessellate(shape, 5.0);

    CHECK(shape.num_verts() == 2);
    CHECK(shape.num_faces() == 1);
  }
  SUBCASE("3D no-op") {
    geom::Shape<Vec<double, 3>> shape{};
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0, 0.0});
    shape.append_vert(Vec{0.0, 1.0, 0.0});
    shape.append_face({0, 1, 2});

    geom::tessellate(shape, 2.0);

    CHECK(shape.num_verts() == 3);
    CHECK(shape.num_faces() == 1);
  }
  SUBCASE("2D split") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{4.0, 0.0});
    shape.append_face({0, 1});

    geom::tessellate(shape, 3.0);

    CHECK(shape.num_verts() == 3);
    CHECK(shape.num_faces() == 2);
    CHECK(shape.vert(2) == Vec{2.0, 0.0});
    // Check that all edges are within limit.
    for (std::size_t i = 0; i < shape.num_faces(); ++i) {
      const auto& v0 = shape.vert(shape.face_verts(i)[0]);
      const auto& v1 = shape.vert(shape.face_verts(i)[1]);
      CHECK(norm(v1 - v0) <= 3.0);
    }
  }
  SUBCASE("3D split") {
    geom::Shape<Vec<double, 3>> shape{};
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{4.0, 0.0, 0.0});
    shape.append_vert(Vec{0.0, 1.0, 0.0});
    shape.append_face({0, 1, 2});

    geom::tessellate(shape, 3.0);

    // Edge (v1,v2) of length sqrt(17) ≈ 4.12 is split first, then
    // edge (v0,v1) of length 4 is split in the next iteration.
    CHECK(shape.num_verts() == 5);
    CHECK(shape.num_faces() == 3);
    CHECK(shape.vert(3) == Vec{2.0, 0.5, 0.0});
    CHECK(shape.vert(4) == Vec{2.0, 0.0, 0.0});
    // Check that no edge exceeds the limit.
    for (std::size_t i = 0; i < shape.num_faces(); ++i) {
      const auto fv = shape.face_verts(i);
      const auto& v0 = shape.vert(fv[0]);
      const auto& v1 = shape.vert(fv[1]);
      const auto& v2 = shape.vert(fv[2]);
      CHECK(norm(v1 - v0) <= 3.0);
      CHECK(norm(v2 - v1) <= 3.0);
      CHECK(norm(v0 - v2) <= 3.0);
    }
  }
  SUBCASE("edge deduplication") {
    // Two triangles sharing a diagonal edge of length sqrt(8) ≈ 2.83.
    geom::Shape<Vec<double, 3>> shape{};
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{2.0, 0.0, 0.0});
    shape.append_vert(Vec{2.0, 2.0, 0.0});
    shape.append_vert(Vec{0.0, 2.0, 0.0});
    shape.append_face({0, 1, 2});
    shape.append_face({0, 2, 3});

    geom::tessellate(shape, 2.0);

    // Only one new vertex should be added for the shared edge.
    CHECK(shape.num_verts() == 5);
    CHECK(shape.num_faces() == 4);
    CHECK(shape.vert(4) == Vec{1.0, 1.0, 0.0});
  }
  SUBCASE("2D multi-segment") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{8.0, 0.0});
    shape.append_face({0, 1});

    geom::tessellate(shape, 3.0);

    // ceil(8/3) = 3 equal-length segments of length 8/3.
    CHECK(shape.num_verts() == 4);
    CHECK(shape.num_faces() == 3);
    for (std::size_t i = 0; i < shape.num_faces(); ++i) {
      const auto& v0 = shape.vert(shape.face_verts(i)[0]);
      const auto& v1 = shape.vert(shape.face_verts(i)[1]);
      CHECK(norm(v1 - v0) <= 3.0 + 1e-12);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
