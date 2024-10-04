/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/uint_utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph coarsening strategy.
enum class CoarseningStrategy : uint8_t {
  /// Sorted Heavy Edge Matching (HEM).
  ///
  /// Nodes are traversed from the lightest to the heaviest edge. For each
  /// node, the node is matched to it's previously unmatched neighbor with the
  /// highest edge weight. If no such neighbor exists, the node remains
  /// unmatched.
  ///
  /// HEM does not have any optimality guarantee, but it's pretty fast.
  heavy_edge_matching,

  /// Sorted Greedy Edge Matching (GEM).
  ///
  /// Edges are traversed from the heaviest to the lightest. Edges with the
  /// same weight are traversed in from the lightest to the heaviest node.
  /// For each edge, the nodes are matched into a single node if both nodes were
  /// not matched before.
  ///
  /// GEM has 1/2-optimality guarantee, but it's slightly slower than HEM.
  greedy_edge_matching,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph coarsening algorithm configuration.
struct CoarseningConfig {
  /// Coarsening strategy. GEM is the default.
  CoarseningStrategy strategy = CoarseningStrategy::greedy_edge_matching;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph coarsener.
///
/// The graph coarsener is used to build a coarse graph from the given fine
/// graph. The coarsening is done by merging fine graph nodes connected by an
/// edge into a single node of the coarse graph.
///
/// The parameters of the coarsening process are determined by the
/// configuration.
class GraphCoarsener final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the coarse graph from the fine graph and weights.
  GraphCoarsener(const WeightedGraph& fine_graph,
                 const WeightArray& fine_weights,
                 const CoarseningConfig& config = {});

  /// Get the coarse graph.
  auto coarse_graph() const noexcept -> const WeightedGraph& {
    return coarse_graph_;
  }

  /// Get the coarse graph node weights.
  auto coarse_weights() const noexcept -> const WeightArray& {
    return coarse_weights_;
  }

  /// Get the fine-to-coarse mapping.
  auto fine_to_coarse() const noexcept -> const NodeMapping& {
    return fine_to_coarse_;
  }

  /// Get the coarse-to-fine mapping.
  auto coarse_to_fine() const noexcept -> const InverseNodeMapping& {
    return coarse_to_fine_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Build the fine-to-coarse mapping using the Heavy Edge Matching.
  // Returns the number of coarse nodes.
  auto match_nodes_hem_(const WeightedGraph& fine_graph,
                        const WeightArray& fine_weights) -> size_t;

  // Build the fine-to-coarse mapping using the Greedy Edge Matching.
  // Returns the number of coarse nodes.
  auto match_nodes_gem_(const WeightedGraph& fine_graph,
                        const WeightArray& fine_weights) -> size_t;

  // Build the graph using the previously constructed fine-to-coarse mapping.
  void build_graph_(const WeightedGraph& fine_graph,
                    const WeightArray& fine_weights,
                    size_t num_coarse_nodes);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  CoarseningConfig config_;
  SplitMix64 rng_;
  NodeMapping fine_to_coarse_;
  InverseNodeMapping coarse_to_fine_;
  WeightedGraph coarse_graph_;
  WeightArray coarse_weights_;

}; // class GraphCoarsener

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
