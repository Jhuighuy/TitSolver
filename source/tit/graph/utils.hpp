/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the edge cut of a graph partitioning.
///
/// The edge cut is the sum of the weights of the edges that connect nodes from
/// different partitions.
///
/// @param[in] graph   Graph.
/// @param[in] weights Node weights.
/// @param[in] parts   Node partitioning.
///
/// @return Edge cut.
constexpr auto edge_cut(const WeightedGraph& graph,
                        const std::vector<part_t>& parts) -> weight_t {
  weight_t edge_cut = 0;
  for (const auto& [edge_weight, node, neighbor] : graph.edges()) {
    if (parts[node] != parts[neighbor]) edge_cut += edge_weight;
  }
  return edge_cut * 2;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if 0
/// Label propagation algorithm.
template<std::ranges::output_range<size_t> Labels>
  requires std::ranges::random_access_range<Labels>
void label_propagation(const Graph& graph,
                       Labels&& labels,
                       size_t max_iter = 100) {
  TIT_ASSUME_UNIVERSAL(Labels, labels);
  TIT_PROFILE_SECTION("Graph::label_propagation()");

  // Initialize the node permutation.
  auto perm = graph.nodes() | std::ranges::to<std::vector>();

  // Run the label propagation algorithm.
  std::minstd_rand rng{};
  for (size_t iter = 0; iter < max_iter; ++iter) {
    // Shuffle the nodes to prevent order bias.
    std::ranges::shuffle(perm, rng);

    // For each node, update it's label based on the most frequent label of it's
    // neighbors.
    bool any_label_updated = false;
    for (const size_t node : perm) {
      if (graph[node].empty()) continue;

      // Count the frequency of each label among the neighbors.
      std::unordered_map<size_t, size_t> label_count; // NOLINT
      for (const auto neighbor : graph[node]) {
        const auto neighbor_label = labels[neighbor];
        label_count[neighbor_label]++;
      }

      // Find the label with the highest frequency (break ties randomly).
      size_t best_label = labels[node];
      size_t max_count = 0;
      for (const auto& [label, count] : label_count) {
        if (count > max_count || (count == max_count && rng() % 2 == 1)) {
          best_label = label;
          max_count = count;
        }
      }

      // Update the label of the current node if it's changed.
      if (labels[node] != best_label) {
        labels[node] = best_label;
        any_label_updated = true;
      }
    }

    // If no labels have been updated, the algorithm converges early.
    if (!any_label_updated) break;
  }
}
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
