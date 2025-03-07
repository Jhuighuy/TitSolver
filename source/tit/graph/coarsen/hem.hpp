/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand.hpp"
#include "tit/core/range.hpp"

#include "tit/graph/coarsen/utils.hpp"
#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Coarsen the graph using Sorted Heavy Edge Matching (HEM) algorithm.
///
/// Nodes are traversed from the lightest to the heaviest edge. For each
/// node, the node is matched to it's previously unmatched neighbor with the
/// highest edge weight. If no such neighbor exists, the node remains
/// unmatched.
///
/// HEM does not have any optimality guarantee, but it's pretty fast.
class CoarsenHEM final {
public:

  /// Construct the coarse graph from the fine graph and weights.
  ///
  /// @param fine_graph     Fine graph.
  /// @param coarse_graph   Coarse graph.
  /// @param coarse_to_fine Coarse-to-fine mapping.
  /// @param fine_to_coarse Fine-to-coarse mapping.
  template<weighted_graph FineGraph,
           weighted_graph CoarseGraph,
           index_range CoarseToFine,
           index_range FineToCoarse>
  static void operator()(const FineGraph& fine_graph,
                         CoarseGraph& coarse_graph,
                         CoarseToFine& coarse_to_fine,
                         FineToCoarse& fine_to_coarse) {
    TIT_PROFILE_SECTION("Graph::CoarsenHEM::operator()");

    SplitMix64 rng{fine_graph.num_nodes()};

    // Construct permutation of the fine graph nodes.
    //
    // We will prioritize the least weigted nodes to reduce the minimal weight
    // of the coarse graph, thus making the weight distribution of the coarse
    // graph more uniform.
    //
    // Equally weighted nodes will be randomly shuffled to avoid biasing.
    auto fine_nodes = fine_graph.nodes() | std::ranges::to<std::vector>();
    par::sort(fine_nodes, {}, [&fine_graph](size_t fine_node) {
      return std::tuple{fine_graph.weight(fine_node),
                        randomized_hash(fine_node)};
    });

    // Build the fine to coarse mapping.
    fine_to_coarse.assign(fine_graph.num_nodes(), npos);
    coarse_to_fine.clear(), coarse_to_fine.reserve(fine_graph.num_nodes());
    for (node_t coarse_node = 0; const auto fine_node : fine_nodes) {
      if (fine_to_coarse[fine_node] != npos) continue;

      // Map the fine node to a new coarse node.
      fine_to_coarse[fine_node] = coarse_node;
      coarse_to_fine.push_back(fine_node);

      // Try to find a neighbor to merge the node with. Find the previously
      // unmatched neighbor with the highest edge weight. If multiple neighbors
      // have the same edge weight, choose the neighbor with the largest node
      // weight, if both are equal, break ties randomly.
      //
      // By removing the heaviest edges, we will hopefully reduce the edge cut
      // at the coarsest level of the graph partitioning, while keeping the node
      // weight distribution is as uniform as possible.
      size_t best_neighbor = npos;
      weight_t best_edge_weight = 0;
      for (const auto& [fine_neighbor, edge_weight] :
           fine_graph.wedges(fine_node)) {
        if (fine_to_coarse[fine_neighbor] != npos) continue;

        if (best_neighbor == npos ||
            greater_or(edge_weight,
                       best_edge_weight,
                       less_or(fine_graph.weight(fine_neighbor),
                               fine_graph.weight(best_neighbor),
                               rng))) {
          best_neighbor = fine_neighbor;
          best_edge_weight = edge_weight;
        }
      }
      if (best_neighbor != npos) {
        fine_to_coarse[best_neighbor] = coarse_node;
        coarse_to_fine.push_back(best_neighbor);
      }

      coarse_node += 1;
    }

    // Build the coarse graph.
    impl::build_coarse_graph(fine_graph,
                             coarse_graph,
                             coarse_to_fine,
                             fine_to_coarse);
  }

}; // class CoarsenHEM

/// @copydoc CoarsenHEM
inline constexpr CoarsenHEM coarsen_hem{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
