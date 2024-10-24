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

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/containers/mdvector.hpp"
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

    // Index the cells that contain the points.
    struct CountAndIndex {
      size_t count = 0;
      size_t flat_index = std::numeric_limits<size_t>::max();
    };
    static Mdvector<CountAndIndex, vec_dim_v<Vec>> cells{};
    cells.assign(grid.num_cells().elems());
    size_t flat_cell_counter = 0;
    {
      TIT_PROFILE_SECTION("GridGraphPartition::operator()::index");
#if 0
      par::for_each(points, [&grid, &flat_cell_counter](const Vec& point) {
        auto& [count, flat_index] = cells[grid.cell_index(point).elems()];
        if (par::compare_exchange(count, 0, 1)) {
          flat_index = par::fetch_and_add(flat_cell_counter, 1);
        } else {
          par::fetch_and_add(count, 1);
        }
      });
#else
      par::for_each(points, [&grid](const Vec& point) {
        auto& [count, _] = cells[grid.cell_index(point).elems()];
        par::fetch_and_add(count, 1);
      });
      for (auto& [count, flat_cell_index] : cells) {
        if (count > 0) flat_cell_index = flat_cell_counter++;
      }
#endif
    }

    // Build the graph connecting the cells.
    constexpr auto MaxNumEdges = 2 * vec_dim_v<Vec>;
    using Edge = std::pair<size_t, size_t>;
    using EdgeMap = SmallVector<Edge, MaxNumEdges>;
    graph::SmallWeightedGraph<MaxNumEdges> graph;
    graph.assign(flat_cell_counter);
    std::vector<graph::weight_t> node_weights(flat_cell_counter);
    {
      TIT_PROFILE_SECTION("GridGraphPartition::operator()::assemble");
      par::for_each(grid.internal_cells(1), [&](const auto& cell_index) {
        const auto& [count, flat_index] = cells[cell_index.elems()];
        if (count == 0) return;

        EdgeMap edges;
        for (size_t d = 0; d < vec_dim_v<Vec>; ++d) {
          for (ssize_t i = -1; i <= 1; i += 2) {
            auto neighbor_cell_index = cell_index;
            neighbor_cell_index[d] += i;
            const auto& [neighbor_count, neighbor_flat_index] =
                cells[neighbor_cell_index.elems()];
            if (neighbor_count == 0) continue;

            const auto edge_weight = count * neighbor_count;
            TIT_ENSURE(
                neighbor_flat_index != flat_index,
                "Neighbor cell index is equal to the current cell index!");
            edges.emplace_back(neighbor_flat_index, edge_weight);
          }
        }

        std::ranges::sort(edges);
        graph.set_bucket(flat_index, edges);
        node_weights[flat_index] = count;
      });
    }

    // Build the partitioning.
    std::vector<size_t> graph_parts(graph.num_nodes());
    graph::partition_metis(graph, node_weights, graph_parts, num_parts);

    // Assign the partitions.
    par::for_each(std::views::enumerate(points), [&](const auto& ip) {
      const auto& [i, point] = ip;
      const auto& cell = cells[grid.cell_index(point).elems()];
      parts[i] = init_part + graph_parts[cell.flat_index];
    });
  }

private:

  real_t size_hint_;

}; // class GridGraphPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
