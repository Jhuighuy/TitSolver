/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/partition/grid_graph_partition.hpp"

#include "tit/graph/partition.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::GridGraphPartition") {
  // Create points on a 8x16 lattice.
  std::array<Vec2D, 128> points{};
  for (size_t i = 0; i < 128; ++i) points[i] = {i % 16, i / 16};

  // Partition the points using the coordinate bisection algorithm.
  // Since here we are testing the geometrical partitioning, we'll use the
  // simplest possible graph partitioning algorithm.
  std::array<size_t, 128> parts{};
  const geom::GridGraphPartition grid_graph_partition{
      /*size_hint=*/2.0,
      graph::UniformPartition{}};
  grid_graph_partition(points, parts, 8);

  // Ensure the resulting partitioning is correct.
  //
  // 0 --------------------------------->
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7    X
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // | 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  // |
  // v
  //   Y
  for (const auto& [part, point] : std::views::zip(parts, points)) {
    CHECK(part == static_cast<size_t>(point[0]) / 2);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
