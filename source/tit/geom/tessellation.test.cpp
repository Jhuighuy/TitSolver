/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <ranges>

#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Surface::tessellate") {
  SUBCASE("2D") {
    // Construct a triangle with integer edge lengths.
    geom::Surface<Vec<double, 2>> surf;
    surf.append_vert({0.0, 0.0});
    surf.append_vert({3.0, 0.0});
    surf.append_vert({0.0, 4.0});
    surf.append_face({0, 1});
    surf.append_face({1, 2});
    surf.append_face({2, 0});

    // Tessellate the surface.
    const auto tess = geom::tessellate(surf, 1.0);

    // Ensure the vertices are correct.
    constexpr auto expected_verts = std::to_array<Vec<double, 2>>({
        {0.0, 0.0},
        {3.0, 0.0},
        {0.0, 4.0},
        {1.0, 0.0},
        {2.0, 0.0},
        {2.4, 0.8},
        {1.8, 1.6},
        {1.2, 2.4},
        {0.6, 3.2},
        {0.0, 3.0},
        {0.0, 2.0},
        {0.0, 1.0},
    });
    REQUIRE(tess.num_verts() == expected_verts.size());
    for (const auto& [vert, expected_vert] :
         std::views::zip(tess.verts(), expected_verts)) {
      CHECK_APPROX_EQ(vert, expected_vert);
    }

    // Ensure the faces are correct.
    constexpr auto expected_faces = std::to_array<std::array<std::size_t, 2>>({
        {0, 3},
        {3, 4},
        {4, 1},
        {1, 5},
        {5, 6},
        {6, 7},
        {7, 8},
        {8, 2},
        {2, 9},
        {9, 10},
        {10, 11},
        {11, 0},
    });
    REQUIRE(tess.num_faces() == expected_faces.size());
    for (const auto& [face_index, expected_face] :
         std::views::enumerate(expected_faces)) {
      CHECK(tess.face_verts(face_index) == expected_face);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
