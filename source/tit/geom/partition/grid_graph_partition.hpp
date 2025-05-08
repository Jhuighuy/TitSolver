/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/inplace_flat_map.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/simple_partition.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partitioning based on a graph partitioning of a grid cell connectivity.
/// @todo Replace `UniformPartition` with a proper graph partitioning function.
template<class Num,
         graph::partition_func GraphPartition = graph::UniformPartition>
class GridGraphPartition final {
public:

  /// Construct a grid graph partitioning function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridGraphPartition(
      Num size_hint,
      GraphPartition graph_partition = {}) noexcept
      : size_hint_{size_hint}, graph_partition_{std::move(graph_partition)} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Partition the points using the grid graph partitioning algorithm.
  template<point_range Points, output_index_range Parts>
    requires std::same_as<point_range_num_t<Points>, Num>
  void operator()(Points&& points,
                  Parts&& parts,
                  size_t num_parts,
                  size_t init_part = 0) const {
    TIT_PROFILE_SECTION("GridGraphPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Points, points);
    TIT_ASSUME_UNIVERSAL(Parts, parts);
    static constexpr auto Dim = point_range_dim_v<Points>;

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(std::size(points) >= num_parts,
               "Number of points cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(points) == std::size(parts),
                 "Size of parts range must be equal to the number of points!");
    }

    // Compute bounding box and initialize the grid. We'll extend the number of
    // cells by one in each direction to avoid the conditionals near boundary.
    //
    // Note: box extension factor "100" does not mean anything particular, it's
    //       just for historical reasons, and can be replaced with any positive
    //       number.
    const auto box = compute_bbox(points).grow(size_hint_ / 100);
    const auto grid = Grid{box}.set_cell_extents(size_hint_).extend(1);

    // Index the cells that contain the points. Thouse would be used as nodes
    // in the graph. We'll use amount of points in each cell as the node weight.
    //
    // Since the typical SPH adjacency graph is heavily connected, we'll use
    // the product of the node weights as the edge weight, as if each particle
    // in the cell is connected to all other particles in neighboring cell.
    struct NodeAndWeight {
      graph::node_t node = npos;
      graph::weight_t weight = 0;
    };
    Mdvector<NodeAndWeight, Dim> cells{grid.num_cells().elems()};
    par::for_each(points, [&grid, &cells](const auto& point) {
      auto& weight = cells[grid.cell_index(point).elems()].weight;
      par::fetch_and_add(weight, 1);
    });
    size_t num_nodes = 0;
    for (auto& [node, weight] : cells) {
      if (weight > 0) node = num_nodes++;
    }

    // Build the graph connecting the cells.
    /// @todo I wish this code to be cleaner. Once we have a proper graph
    ///       library, we can simplify this code.
    static constexpr auto MaxNumEdges = 2 * Dim;
    using EdgeMap = InplaceFlatMap<size_t, size_t, MaxNumEdges>;
    graph::CapWeightedGraph<MaxNumEdges> graph(num_nodes);
    std::vector<graph::weight_t> node_weights(num_nodes);
    par::for_each( //
        grid.cells(1),
        [&graph, &node_weights, &cells](const auto& cell_index) {
          const auto& [node, weight] = cells[cell_index.elems()];
          if (weight == 0) return;
          TIT_ASSERT(node != npos, "Missing node!");

          // Build the edges.
          EdgeMap edges{};
          for (size_t d = 0; d < Dim; ++d) {
            for (ssize_t i = -1; i <= 1; i += 2) {
              auto neighbor_cell_index = cell_index;
              neighbor_cell_index[d] += i;
              const auto& [neighbor, neighbor_weight] =
                  cells[neighbor_cell_index.elems()];
              if (neighbor_weight == 0) continue;

              const auto edge_weight = weight * neighbor_weight;
              edges.emplace(neighbor, edge_weight);
            }
          }

          // Set the node edges and weight.
          graph.set_bucket(node, edges);
          node_weights[node] = weight;
        });

    // Build the graph partitioning.
    std::vector<size_t> graph_parts(graph.num_nodes());
    graph_partition_(graph, node_weights, graph_parts, num_parts);

    // Propagate the partitions to the points.
    par::transform( //
        points,
        std::begin(parts),
        [&grid, &cells, &graph_parts, init_part](const auto& point) {
          const auto node = cells[grid.cell_index(point).elems()].node;
          TIT_ASSERT(node != npos, "Missing node!");
          return init_part + graph_parts[node];
        });
  }

private:

  Num size_hint_;
  [[no_unique_address]] GraphPartition graph_partition_;

}; // class GridGraphPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
