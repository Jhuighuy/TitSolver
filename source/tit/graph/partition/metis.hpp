/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning using the Metis library.
class MetisPartition final {
public:

  /// Partition the graph using the Metis library.
  template<weighted_graph Graph, node_parts<Graph> Parts>
  void operator()(const Graph& graph, Parts&& parts, size_t num_parts) const {
    TIT_PROFILE_SECTION("Graph::MetisPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(num_parts <= graph.num_nodes(),
               "Number of nodes cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(parts) == graph.num_nodes(),
                 "Size of parts range must be equal to the number of nodes!");
    }

    // Convert the graph to the Metis format.
    std::vector<metis_index_t_> node_weights;
    std::vector<metis_index_t_> edge_ranges{0};
    std::vector<metis_index_t_> edge_neighbors;
    std::vector<metis_index_t_> edge_weights;
    for (const auto& [node, weight] : graph.wnodes()) {
      node_weights.push_back(to_metis_(weight));
      for (const auto& [neighbor, edge_weight] : graph.wedges(node)) {
        edge_neighbors.push_back(to_metis_(neighbor));
        edge_weights.push_back(to_metis_(edge_weight));
      }
      edge_ranges.push_back(to_metis_(edge_ranges.size()));
    }

    // Partition the graph.
    std::vector<metis_index_t_> metis_parts(graph.num_nodes());
    run_metis_(node_weights,
               edge_ranges,
               edge_neighbors,
               edge_weights,
               metis_parts,
               to_metis_(num_parts));

    // Copy the partitioning.
    std::ranges::copy(metis_parts, std::begin(parts));
  }

private:

  // Metis index type.
  using metis_index_t_ = int32_t;

  // Cast to Metis index type.
  static constexpr auto to_metis_(std::integral auto val) noexcept
      -> metis_index_t_ {
    return static_cast<metis_index_t_>(val);
  }

  // Run Metis.
  static void run_metis_(std::span<metis_index_t_> node_weights,
                         std::span<metis_index_t_> edge_ranges,
                         std::span<metis_index_t_> edge_neighbors,
                         std::span<metis_index_t_> edge_weights,
                         std::span<metis_index_t_> parts,
                         metis_index_t_ num_parts);

}; // class MetisPartition

/// Metis graph partitioning.
inline constexpr MetisPartition metis_partition{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
