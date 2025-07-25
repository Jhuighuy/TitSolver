/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <span>
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
namespace impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Graph in ELL format.
template<size_t MaxNumNodeEdges>
class ELLGraph final {
public:

  // Construct a graph with provided number of nodes.
  constexpr explicit ELLGraph(size_t num_nodes)
      : weights_(num_nodes, 1), adj_node_counts_(num_nodes),
        adj_nodes_(num_nodes) {}

  // Get the number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return weights_.size();
  }

  // Access the node weights.
  constexpr auto weight(size_t node) const noexcept -> size_t {
    TIT_ASSERT(node < num_nodes(), "Node is out of range!");
    return weights_[node];
  }
  constexpr void set_weight(size_t node, size_t weight) noexcept {
    TIT_ASSERT(weight > 0, "Weight must be positive!");
    TIT_ASSERT(node < num_nodes(), "Node is out of range!");
    weights_[node] = weight;
  }

  // Access the adjacency data.
  constexpr auto adjacent(size_t node) const noexcept
      -> std::span<const size_t> {
    TIT_ASSERT(node < num_nodes(), "Node is out of range!");
    return std::span{adj_nodes_[node]}.subspan(0, adj_node_counts_[node]);
  }
  constexpr void connect(size_t node, size_t adj_node) noexcept {
    TIT_ASSERT(node < num_nodes(), "Node is out of range!");
    TIT_ASSERT(adj_node < num_nodes(), "Node is out of range!");
    TIT_ASSERT(adj_node_counts_[node] < MaxNumNodeEdges, "Too many edges!");
    adj_nodes_[node][adj_node_counts_[node]++] = adj_node;
  }

private:

  std::vector<size_t> weights_;
  std::vector<size_t> adj_node_counts_;
  std::vector<std::array<size_t, MaxNumNodeEdges>> adj_nodes_;

}; // class ELLGraph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @todo Replace with a proper graph partitioning algorithm.
template<size_t MaxNumNodeEdges>
constexpr void partition(const ELLGraph<MaxNumNodeEdges>& graph,
                         auto& parts,
                         size_t num_parts) {
  const auto num_nodes = graph.num_nodes();
  const auto part_size = num_nodes / num_parts;
  const auto remainder = num_nodes % num_parts;
  for (size_t part = 0; part < num_parts; ++part) {
    const auto first = part * part_size + std::min(part, remainder);
    const auto last = (part + 1) * part_size + std::min(part + 1, remainder);
    for (size_t i = first; i < last; ++i) parts[i] = part;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl

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
    //
    // Note: box extension factor "100" does not mean anything particular, it's
    //       just for historical reasons, and can be replaced with any positive
    //       number.
    const auto box = compute_bbox(points).grow(size_hint_ / 100);
    const auto grid = Grid{box}.set_cell_extents(size_hint_).extend(1);

    // Index the cells that contain points. Those would be used as nodes of the
    // graph. We'll use amount of points in each cell as the node weight.
    struct NodeAndWeight {
      size_t node = 0;
      size_t weight = 0;
    };
    Mdvector<NodeAndWeight, Dim> cells{grid.num_cells().elems()};
    for (const auto& point : points) {
      const auto cell_index = grid.cell_index(point);
      ++cells[cell_index.elems()].weight;
    }
    size_t num_nodes = 0;
    for (auto& [node, weight] : cells) {
      if (weight != 0) node = num_nodes++;
    }

    // Build the cell adjacency graph.
    constexpr auto MaxNumEdges = pow(3UZ, Dim) - 1;
    impl::ELLGraph<MaxNumEdges> graph(num_nodes);
    for (const auto& cell_index : grid.cells(1)) {
      const auto& [node, weight] = cells[cell_index.elems()];
      if (weight == 0) continue;

      graph.set_weight(node, weight);

      for (const auto& adj_cell_index : grid.cells_inclusive(cell_index, 1)) {
        const auto& [adj_node, adj_weight] = cells[adj_cell_index.elems()];
        if (adj_node == node || adj_weight == 0) continue;

        graph.connect(node, adj_node);
      }
    }

    // Build the graph partitioning.
    std::vector<size_t> graph_parts(num_nodes);
    impl::partition(graph, graph_parts, num_parts);

    // "Interpolate" graph partitioning to the points.
    for (auto&& [point, part] : std::views::zip(points, parts)) {
      const auto node = cells[grid.cell_index(point).elems()].node;
      part = init_part + graph_parts[node];
    }
  }

private:

  Num size_hint_;

}; // class GridGraphPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
