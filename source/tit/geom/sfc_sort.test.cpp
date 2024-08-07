/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/sfc_sort.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Sketch of a Morton curve for a 8x8 lattice:
//
//       0    1    2    3      4    5    6    7
//   0 --|----|----|----|------|----|----|----|--->
//   |                                             X
// 0 -   00 - 01   04 - 05     16 - 17   20 - 21
//   |      /    /    /      /    /    /    /
// 1 -   02 - 03   06 - 07  |  18 - 19   22 - 23
//   |                 /    |                /
//   |     ------------     |    ------------
//   |    /                 |   /
// 2 -   08 - 09   12 - 13  |  24 - 25   28 - 29
//   |      /    /    /    /      /    /    /
// 3 -   10 - 11   14 - 15     26 - 27   30 - 31
//   |                                       /
//   |      ---------------------------------
//   |     /
// 4 -   32 - 33   36 - 37     48 - 49   52 - 53
//   |      /    /    /      /    /    /    /
// 5 -   34 - 35   38 - 39  |  50 - 51   54 - 55
//   |                 /    |                /
//   |     ------------     |    ------------
//   |    /                 |   /
// 6 -   40 - 41   44 - 45  |  56 - 57   60 - 61
//   |      /    /    /    /      /    /    /
// 7 -   42 - 43   46 - 47     58 - 59   62 - 63
//   |
//   v
//    Y
//
constexpr auto sorted_8x8_lattice_points = std::to_array<Vec2D>({
    {0, 0}, {0, 1}, {1, 0}, {1, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3},
    {2, 0}, {2, 1}, {3, 0}, {3, 1}, {2, 2}, {2, 3}, {3, 2}, {3, 3},
    {0, 4}, {0, 5}, {1, 4}, {1, 5}, {0, 6}, {0, 7}, {1, 6}, {1, 7},
    {2, 4}, {2, 5}, {3, 4}, {3, 5}, {2, 6}, {2, 7}, {3, 6}, {3, 7},
    {4, 0}, {4, 1}, {5, 0}, {5, 1}, {4, 2}, {4, 3}, {5, 2}, {5, 3},
    {6, 0}, {6, 1}, {7, 0}, {7, 1}, {6, 2}, {6, 3}, {7, 2}, {7, 3},
    {4, 4}, {4, 5}, {5, 4}, {5, 5}, {4, 6}, {4, 7}, {5, 6}, {5, 7},
    {6, 4}, {6, 5}, {7, 4}, {7, 5}, {6, 6}, {6, 7}, {7, 6}, {7, 7},
});

TEST_CASE("geom::MortonCurveSort") {
  // Create points on a 8x8 lattice.
  std::array<Vec2D, 64> points;
  for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

  // Sort points using Morton curve.
  const geom::MortonCurveSort sfc{points};

  // Ensure the resulting permutation is correct.
  for (const auto [i, p] : std::views::enumerate(sfc.perm())) {
    CHECK(all(points[p] == sorted_8x8_lattice_points[i]));
  }
  for (const auto [i, ip] : std::views::enumerate(sfc.iperm())) {
    CHECK(all(points[i] == sorted_8x8_lattice_points[ip]));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
