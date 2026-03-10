/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/partition/pixelated_partition.hpp"
#include "tit/geom/partition/sort_partition.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::PixelatedPartition") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // With size_hint=4.0 the bounding box (grown by 2.0 on each side) spans
  // 7 units per axis, which splits into exactly 2 cells of 3.5 units each.
  // Each 2×2 block of lattice points therefore maps to the same pixel:
  //
  //   (x=0..1, y=0..1) → pixel (0,0)   (x=2..3, y=0..1) → pixel (1,0)
  //   (x=0..1, y=2..3) → pixel (0,1)   (x=2..3, y=2..3) → pixel (1,1)
  //
  const geom::PixelatedPartition ppf{4.0, geom::morton_curve_partition};

  std::array<size_t, 16> parts{};
  ppf(points, parts, 4);

  // The 4x4 lattice is divided into four 2x2 pixel blocks:
  //   Block A: indices {0,1,4,5}   (x=0..1, y=0..1)
  //   Block B: indices {2,3,6,7}   (x=2..3, y=0..1)
  //   Block C: indices {8,9,12,13} (x=0..1, y=2..3)
  //   Block D: indices {10,11,14,15}(x=2..3, y=2..3)
  //
  // All points within each block must share the same partition index.
  for (const size_t i : {1, 4, 5}) CHECK(parts[i] == parts[0]);
  for (const size_t i : {3, 6, 7}) CHECK(parts[i] == parts[2]);
  for (const size_t i : {9, 12, 13}) CHECK(parts[i] == parts[8]);
  for (const size_t i : {11, 14, 15}) CHECK(parts[i] == parts[10]);

  // The four blocks must land in four distinct partitions.
  CHECK(parts[0] != parts[2]);
  CHECK(parts[0] != parts[8]);
  CHECK(parts[0] != parts[10]);
  CHECK(parts[2] != parts[8]);
  CHECK(parts[2] != parts[10]);
  CHECK(parts[8] != parts[10]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
