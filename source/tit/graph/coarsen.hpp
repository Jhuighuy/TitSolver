/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Build the coarse graph from the fine graph and coarse-fine mapping.
template<weighted_graph FineGraph,
         weighted_graph CoarseGraph,
         node_mapping<CoarseGraph, FineGraph> CoarseToFine,
         node_mapping<FineGraph, CoarseGraph> FineToCoarse>
void build_coarse_graph(const FineGraph& fine_graph,
                        CoarseGraph& coarse_graph,
                        CoarseToFine&& coarse_to_fine,
                        FineToCoarse&& fine_to_coarse) {
  TIT_ASSUME_UNIVERSAL(CoarseToFine, coarse_to_fine);
  TIT_ASSUME_UNIVERSAL(FineToCoarse, fine_to_coarse);
  coarse_graph.clear();
  equality_ranges(
      coarse_to_fine,
      [&fine_graph, &fine_to_coarse, &coarse_graph](auto fine_nodes) {
        weight_t coarse_weight = 0;
        SmallFlatMap<node_t, weight_t, 32> coarse_neighbors{};
        for (const auto fine_node : fine_nodes) {
          coarse_weight += fine_graph.weight(fine_node);
          for (const auto& [fine_neighbor, fine_edge_weight] :
               fine_graph.wedges(fine_node)) {
            const auto coarse_neighbor = fine_to_coarse[fine_neighbor];
            coarse_neighbors[coarse_neighbor] += fine_edge_weight;
          }
        }
        coarse_graph.append_node(coarse_weight, coarse_neighbors);
      },
      /*pred=*/{},
      /*proj=*/[&fine_to_coarse](node_t fn) { return fine_to_coarse[fn]; });
}

} // namespace impl

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
           node_mapping<CoarseGraph, FineGraph> CoarseToFine,
           node_mapping<FineGraph, CoarseGraph> FineToCoarse>
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
    par::sort(fine_nodes, std::less{}, [&fine_graph](size_t fine_node) {
      return std::tuple{fine_graph.weight(fine_node),
                        randomized_hash(fine_node)};
    });

    // Build the fine to coarse mapping.
    node_t coarse_node = 0;
    fine_to_coarse.assign(fine_graph.num_nodes(), npos);
    coarse_to_fine.clear(), coarse_to_fine.reserve(fine_graph.num_nodes());
    for (const auto fine_node : fine_nodes) {
      if (fine_to_coarse[fine_node] != npos) continue;

      // Map the fine node to a new coarse node.
      fine_to_coarse[fine_node] = coarse_node;
      coarse_to_fine.push_back(fine_node);
      coarse_node += 1;

      // Try to find a neighbor to merge the node with. Find the previously
      // unmatched neighbor with the highest edge weight. If multiple neighbors
      // have the same edge weight, choose the neighbor with the largest node
      // weight, if both are equal, break ties randomly.
      //
      // By removing the heaviest edges, we will hopefully reduce the edge cut
      // at the coarsest level of the graph partitioning, while keeping the node
      // weight distribution is as uniform as possible.
      //
      /// @todo Rewrite using `std::ranges::max_element`.
      size_t best_neighbor = npos;
      weight_t best_edge_weight = 0;
      for (const auto& [fine_neighbor, edge_weight] :
           fine_graph.wedges(fine_node)) {
        if (fine_to_coarse[fine_neighbor] != npos) continue;

        if (greater_or(edge_weight,
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
           node_mapping<CoarseGraph, FineGraph> CoarseToFine,
           node_mapping<FineGraph, CoarseGraph> FineToCoarse>
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

/// Graph coarsening function.
template<class Coarsen>
concept coarsen_func = std::same_as<Coarsen, CoarsenHEM> || //
                       std::same_as<Coarsen, CoarsenGEM>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
