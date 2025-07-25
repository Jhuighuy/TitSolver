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
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partitioning based on a graph partitioning of a grid cell connectivity.
template<class Num>
class GridGraphPartition final {
public:

  /// Construct a grid graph partitioning function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridGraphPartition(Num size_hint) noexcept
      : size_hint_{size_hint} {
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
    const auto box = compute_bbox(points).grow(size_hint_ / 2);
    const auto grid = Grid{box}.set_cell_extents(size_hint_).extend(1);

    // Index the cells that contain points. Those would be used as nodes of the
    // graph. We'll use amount of points in each cell as the node weight.
    struct NodeAndWeight {
      size_t node = 0;
      uint32_t weight = 0;
    };
    Mdvector<NodeAndWeight, Dim> cells{grid.num_cells().elems()};
    for (size_t counter = 0; const auto& point : points) {
      const auto cell_index = grid.cell_index(point);
      auto& [node, weight] = cells[cell_index.elems()];
      if (weight == 0) node = counter++;
      ++weight;
    }

    // Build the cell adjacency graph.
    std::vector<uint32_t> node_weights;
    std::vector<size_t> node_ptrs{0};
    std::vector<size_t> node_adjacency;
    for (const auto& cell_index : grid.cells(1)) {
      const auto& [node, weight] = cells[cell_index.elems()];
      if (weight == 0) return;

      for (const auto& adj_cell_index : grid.cells_inclusive(cell_index, 1)) {
        const auto& [adj_node, adj_weight] = cells[adj_cell_index.elems()];
        if (adj_node == node || adj_weight == 0) continue;

        node_adjacency.push_back(adj_node);
      }

      node_weights.push_back(weight);
      node_ptrs.push_back(node_adjacency.size());
    }

    // Build the graph partitioning.
    std::vector<size_t> node_parts(node_ptrs.size() - 1);
    partition_graph_(node_ptrs,
                     node_weights,
                     node_adjacency,
                     node_parts,
                     num_parts);

    // Propagate the partitions to the points.
    for (auto&& [point, part] : std::views::zip(points, parts)) {
      const auto node = cells[grid.cell_index(point).elems()].node;
      part = init_part + node_parts[node];
    }
  }

private:

  /// @todo Replace with proper graph partitioning algorithm.
  static void partition_graph_(const auto& node_ptrs,
                               const auto& /*node_weights*/,
                               const auto& /*node_adjacency*/,
                               auto& parts,
                               size_t num_parts) {
    const auto num_nodes = node_ptrs.size() - 1;
    const auto part_size = num_nodes / num_parts;
    const auto remainder = num_nodes % num_parts;
    for (size_t part = 0; part < num_parts; ++part) {
      const auto first = part * part_size + std::min(part, remainder);
      const auto last = (part + 1) * part_size + std::min(part + 1, remainder);
      for (size_t i = first; i < last; ++i) parts[i] = part;
    }
  }

  Num size_hint_;

}; // class GridGraphPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
