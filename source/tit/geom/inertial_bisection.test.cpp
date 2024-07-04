/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/inertial_bisection.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::InertialBisection") {
  // Create points on a 8x8 lattice.
  std::array<Vec2D, 64> points{};
  for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

  // Partition the points using the inertial bisection algorithm.
  std::array<size_t, 64> parts{};
  const geom::InertialBisection ib{points, parts, 4};

  // Ensure the resulting partitioning is correct.
  // We shall have 4 parts in each quadrant:
  //
  // 0 ----------------->
  // | 0 0 0 0 2 2 2 2    Y
  // | 0 0 0 0 2 2 2 2
  // | 0 0 0 0 2 2 2 2
  // | 0 0 0 0 2 2 2 2
  // | 1 1 1 1 3 3 3 3
  // | 1 1 1 1 3 3 3 3
  // | 1 1 1 1 3 3 3 3
  // | 1 1 1 1 3 3 3 3
  // |
  // v
  //  X
  for (const auto& [part, point] : std::views::zip(parts, points)) {
    const auto pi = static_vec_cast<size_t>(point);
    const auto quadrant = pi[1] / 4 + pi[0] / 4 * 2;
    CHECK(part == quadrant);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
