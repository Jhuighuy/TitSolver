/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <functional>
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
/// Edges are traversed from the heaviest to the lightest. Edges with the
/// same weight are traversed in from the lightest to the heaviest node.
/// For each edge, the nodes are matched into a single node if both nodes were
/// not matched before.
///
/// GEM has 1/2-optimality guarantee, but it's slightly slower than HEM.
class CoarsenGEM final {
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
    TIT_PROFILE_SECTION("Graph::CoarsenGEM::operator()");

    // Construct permutation of the fine graph edges.
    //
    // We will prioritize the heaviest edges to reduce the total edge weight
    // of the coarse graph.
    //
    // Among equally weighted edges, we will prioritize the edges with the
    // smallest node weights to reduce the minimal node weight of the coarse
    // graph, thus making the weight distribution of the coarse graph more
    // uniform.
    //
    // Equally weighted edges will be randomly shuffled to avoid biasing.
    auto fine_edges = fine_graph.wedges() | std::ranges::to<std::vector>();
    par::sort(fine_edges, std::greater{}, [&fine_graph](const wedge_t& we) {
      const auto& [fine_node, fine_neighbor, edge_weight] = we;
      return std::tuple{
          edge_weight,
          std::min(fine_graph.weight(fine_neighbor),
                   fine_graph.weight(fine_node)),
          randomized_hash(fine_neighbor, fine_node),
      };
    });

    // Build the fine to coarse mapping.
    //
    // Merge pairs of nodes that are connected by an edge, if both nodes are
    // not already mapped to a coarse node. After merging the edges, keep the
    // unmerged nodes as they are.
    node_t coarse_node = 0;
    fine_to_coarse.assign(fine_graph.num_nodes(), npos);
    coarse_to_fine.clear(), coarse_to_fine.reserve(fine_graph.num_nodes());
    for (const auto& [fine_node, fine_neighbor, _] : fine_edges) {
      if (fine_to_coarse[fine_node] != npos) continue;
      if (fine_to_coarse[fine_neighbor] != npos) continue;

      fine_to_coarse[fine_node] = coarse_node;
      fine_to_coarse[fine_neighbor] = coarse_node;
      coarse_to_fine.push_back(fine_node);
      coarse_to_fine.push_back(fine_neighbor);
      coarse_node += 1;
    }
    for (const auto fine_node : fine_graph.nodes()) {
      if (fine_to_coarse[fine_node] != npos) continue;

      fine_to_coarse[fine_node] = coarse_node;
      coarse_to_fine.push_back(fine_node);
      coarse_node += 1;
    }

    // Build the coarse graph.
    impl::build_coarse_graph(fine_graph,
                             coarse_graph,
                             coarse_to_fine,
                             fine_to_coarse);
  }

}; // class CoarsenGEM

/// @copydoc CoarsenGEM
inline constexpr CoarsenGEM coarsen_gem{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
