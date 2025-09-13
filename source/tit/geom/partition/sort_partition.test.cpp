/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/partition/sort_partition.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::MortonCurvePartition") {
  // Create points on a 8x8 lattice.
  std::array<Vec2D, 64> points{};
  for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

  // Partition the points using the Hilbert curve algorithm.
  std::array<size_t, 64> parts{};
  geom::morton_curve_partition(points, parts, 16);

  // Ensure the resulting partitioning is correct.
  //
  //   |   0    1    2    3    4    5    6    7
  // --+---|----|----|----|----|----|----|----|--->
  //   |                                            X
  // 0 +   00--.00  .01--.01   04--.04  .05--.05
  //   |      /    /    /     /   /    /    /
  // 1 +   00'--00'  01'--01 | 04'--04'  05'--05
  //   |    .-------------'  |  .-------------'
  // 2 +   02--.02  .03--.03 | 06--.06  .07--.07
  //   |      /    /    /   /     /    /    /
  // 3 +   02'--02'  03'--03   06'--06'  07'--07
  //   |    .---------------------------------'
  // 4 +   08--.08  .09--.09   12--.12  .13--.13
  //   |      /    /    /     /   /    /    /
  // 5 +   08'--08'  09'--09 | 12'--12'  13'--13
  //   |    .-------------'  |  .-------------'
  // 6 +   10--.10  .11--.11 | 14--.14  .15--.15
  //   |      /    /    /   /     /    /    /
  // 7 +   10'--10'  11'--11   14'--14'  15'--15
  //   |
  //   v
  //     Y
  //
  CHECK_RANGE_EQ(parts, {0,  0,  1,  1,  4,  4,  5,  5,  //
                         0,  0,  1,  1,  4,  4,  5,  5,  //
                         2,  2,  3,  3,  6,  6,  7,  7,  //
                         2,  2,  3,  3,  6,  6,  7,  7,  //
                         8,  8,  9,  9,  12, 12, 13, 13, //
                         8,  8,  9,  9,  12, 12, 13, 13, //
                         10, 10, 11, 11, 14, 14, 15, 15, //
                         10, 10, 11, 11, 14, 14, 15, 15});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::HilbertCurvePartition") {
  // Create points on a 8x8 lattice.
  std::array<Vec2D, 64> points{};
  for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

  // Partition the points using the Morton curve algorithm.
  std::array<size_t, 64> parts{};
  geom::hilbert_curve_partition(points, parts, 16);

  // Ensure the resulting partitioning is correct.
  //
  //   |   0    1    2    3    4    5    6    7
  // --+---|----|----|----|----|----|----|----|--->
  //   |                                            X
  // 0 +   00   00===01===01   14===14===15   15
  //   |   ||   ||        ||   ||        ||   ||
  // 1 +   00===00   01===01   14===14   15===15
  //   |             ||             ||
  // 2 +   03===03   02===02   13===13   12===12
  //   |   ||   ||        ||   ||        ||   ||
  // 3 +   03   03===02===02   13===13===12   12
  //   |   ||                                 ||
  // 4 +   04===04   07===07===08===08   11===11
  //   |        ||   ||             ||   ||
  // 5 +   04===04   07===07   08===08   11===11
  //   |   ||             ||   ||             ||
  // 6 +   05   05===06   06   09   09===10   10
  //   |   ||   ||   ||   ||   ||   ||   ||   ||
  // 7 +   05===05   06===06   09===09   10===10
  //   |
  //   v
  //     Y
  //
  CHECK_RANGE_EQ(parts, {0, 0, 1, 1, 14, 14, 15, 15, //
                         0, 0, 1, 1, 14, 14, 15, 15, //
                         3, 3, 2, 2, 13, 13, 12, 12, //
                         3, 3, 2, 2, 13, 13, 12, 12, //
                         4, 4, 7, 7, 8,  8,  11, 11, //
                         4, 4, 7, 7, 8,  8,  11, 11, //
                         5, 5, 6, 6, 9,  9,  10, 10, //
                         5, 5, 6, 6, 9,  9,  10, 10});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
