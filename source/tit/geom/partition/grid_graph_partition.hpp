/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/partition.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partitioning based on a graph partitioning of a grid cell connectivity.
class GridGraphPartition final {
public:

  /// Construct a grid graph partitioning function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr /*explicit*/ GridGraphPartition( //
      real_t size_hint = 2.0 * 0.6 / 80.0)
      : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Partition the points using the grid graph partitioning algorithm.
  template<point_range Points, output_index_range Parts>
  void operator()(Points&& points,
                  Parts&& parts,
                  size_t num_parts,
                  size_t init_part = 0) const {
    TIT_PROFILE_SECTION("GridGraphPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Parts, parts);
    using Vec = point_range_vec_t<Points>;

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(std::size(points) >= num_parts,
               "Number of points cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(points) == std::size(parts),
                 "Size of parts range must be equal to the number of points!");
    }

    // Compute bounding box and initialize the grid.
    // We'll extend the number of cells by one in each direction to avoid the
    // conditionals near boundary.
    const auto box = compute_bbox(points).grow(size_hint_ / 100);
    const auto grid = Grid{box}.set_cell_extents(size_hint_).extend(1);

    struct CountAndIndex {
      size_t count = 0;
      size_t flat_index = std::numeric_limits<size_t>::max();
    };
    Mdvector<CountAndIndex, vec_dim_v<Vec>> cells{};
    /// @todo We need a convenient way for this.
    std::apply([&cells](auto... es) { cells.assign(es...); },
               to_array(grid.num_cells()));
    const auto cell_at = [&cells](const auto& cell_index) -> CountAndIndex& {
      return std::apply([&cells](auto... is) -> auto& { return cells[is...]; },
                        to_array(cell_index));
    };

    // Index the cells that contain the points.
    par::for_each(points, [&grid, &cells, &cell_at](const Vec& point) {
      auto& cell = cell_at(grid.cell_index(point));
      par::fetch_and_add(cell.count, 1);
    });

    // Assign flat indices to the cells that have points.
    for (size_t cell_flat_index = 0; auto& cell : cells) {
      if (cell.count > 0) cell.flat_index = cell_flat_index++;
    }

    // Build the graph connecting the cells.
    using EdgeMap = boost::container::small_vector< //
        std::pair<size_t, graph::weight_t>,
        2 * vec_dim_v<Vec>>;
    graph::WeightedGraph graph;
    std::vector<graph::weight_t> node_weights;
    for (const auto& cell_index : grid.internal_cells(1)) {
      const auto& cell = cell_at(cell_index);
      if (cell.count == 0) continue;

      EdgeMap edges;
      for (size_t d = 0; d < vec_dim_v<Vec>; ++d) {
        for (ssize_t i = -1; i <= 1; i += 2) {
          auto neighbor_cell_index = cell_index;
          neighbor_cell_index[d] += i;
          const auto& neighbor_cell = cell_at(neighbor_cell_index);
          if (neighbor_cell.count == 0) continue;

          const auto edge_weight = cell.count * neighbor_cell.count;
          edges.emplace_back(neighbor_cell.flat_index, edge_weight);
        }
      }

      std::ranges::sort(edges);
      graph.append_bucket(edges);
      node_weights.push_back(cell.count);
    }

    // Build the partitioning.
    std::vector<size_t> graph_parts(graph.num_nodes());
    graph::partition_metis(graph, node_weights, graph_parts, num_parts);

    // Assign the partitions.
    par::for_each(std::views::enumerate(points), [&](const auto& ip) {
      const auto& [i, point] = ip;
      const auto& cell = cell_at(grid.cell_index(point));
      parts[i] = init_part + graph_parts[cell.flat_index];
    });
  }

private:

  real_t size_hint_;

}; // class GridGraphPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
