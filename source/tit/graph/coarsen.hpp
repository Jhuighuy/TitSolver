/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand_utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Build the coarse graph from the fine graph and coarse to fine mapping.
template<weighted_graph FineGraph,
         node_weights<FineGraph> FineWeights,
         weighted_graph CoarseGraph,
         node_weights<FineGraph> CoarseWeights,
         node_mapping<CoarseGraph, FineGraph> CoarseToFine,
         node_mapping<FineGraph, CoarseGraph> FineToCoarse>
void build_coarse_graph(const FineGraph& fine_graph,
                        const FineWeights& fine_weights,
                        CoarseGraph& coarse_graph,
                        CoarseWeights& coarse_weights,
                        const CoarseToFine& coarse_to_fine,
                        const FineToCoarse& fine_to_coarse,
                        size_t num_coarse_nodes) {
  // Reserve space for the coarse graph.
  coarse_graph.clear();
  coarse_weights.clear(), coarse_weights.reserve(num_coarse_nodes);

  // Build the coarse graph.
  equality_ranges(
      coarse_to_fine,
      [&fine_graph,
       &fine_weights,
       &fine_to_coarse,
       &coarse_graph,
       &coarse_weights]<class Iter>(std::ranges::subrange<Iter> fine_nodes) {
        SmallFlatMap<size_t, weight_t, 32> coarse_neighbors{};
        weight_t coarse_weight = 0;
        for (const auto fine_node : fine_nodes) {
          for (const auto& [fine_neighbor, //
                            fine_edge_weight] : fine_graph[fine_node]) {
            const auto coarse_neighbor = fine_to_coarse[fine_neighbor];
            coarse_neighbors[coarse_neighbor] += fine_edge_weight;
          }
          coarse_weight += fine_weights[fine_node];
        }
        coarse_graph.append_bucket(coarse_neighbors);
        coarse_weights.push_back(coarse_weight);
      },
      /*pred=*/{},
      /*proj=*/[&fine_to_coarse](size_t fn) { return fine_to_coarse[fn]; });
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Coarse the graph using Sorted Heavy Edge Matching (HEM) algorithm.
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
  /// @param fine_weights   Fine graph node weights.
  /// @param coarse_graph   Coarse graph.
  /// @param coarse_weights Coarse graph node weights.
  /// @param coarse_to_fine Coarse-to-fine mapping.
  /// @param fine_to_coarse Fine-to-coarse mapping.
  template<weighted_graph FineGraph,
           node_weights<FineGraph> FineWeights,
           weighted_graph CoarseGraph,
           node_weights<FineGraph> CoarseWeights,
           node_mapping<CoarseGraph, FineGraph> CoarseToFine,
           node_mapping<FineGraph, CoarseGraph> FineToCoarse>
  static void operator()(const FineGraph& fine_graph,
                         const FineWeights& fine_weights,
                         CoarseGraph& coarse_graph,
                         CoarseWeights& coarse_weights,
                         CoarseToFine& coarse_to_fine,
                         FineToCoarse& fine_to_coarse) {
    TIT_PROFILE_SECTION("Graph::CoarsenHEM::operator()");
    TIT_ASSERT(fine_graph.num_nodes() == fine_weights.size(),
               "Invalid number of the fine graph weights!");

    SplitMix64 rng{fine_graph.num_nodes()};

    // Construct permutation of the fine graph nodes.
    //
    // Since the node weights of the coarse graph are sums of the weights of the
    // corresponding node weights of the fine graph, we will prioritize the
    // least weigted nodes to reduce the minimal weight of the coarse graph,
    // thus making the weight distribution of the coarse graph more uniform.
    //
    // Equally weighted nodes will be randomly shuffled to avoid biasing.
    auto fine_nodes = fine_graph.nodes() | std::ranges::to<std::vector>();
    const auto node_priority = [&fine_weights](size_t fn) {
      return fine_weights[fn];
    };
    std::ranges::sort(fine_nodes, std::less{}, node_priority);
    sorted_equality_ranges(
        fine_nodes,
        [&rng](std::ranges::view auto fns) { std::ranges::shuffle(fns, rng); },
        std::less{},
        node_priority);

    // Build the fine to coarse mapping.
    size_t num_coarse_nodes = 0;
    fine_to_coarse.assign(fine_graph.num_nodes(), npos_node);
    coarse_to_fine.clear(), coarse_to_fine.reserve(fine_graph.num_nodes());
    for (const auto fine_node : fine_nodes) {
      if (fine_to_coarse[fine_node] != npos_node) continue;

      // Map the fine node to a new coarse node.
      const auto coarse_node = num_coarse_nodes++;
      fine_to_coarse[fine_node] = coarse_node;
      coarse_to_fine.push_back(fine_node);

      // Try to find a neighbor to merge the node with. Find the previously
      // unmatched neighbor with the highest edge weight. If multiple neighbors
      // have the same edge weight, choose the neighbor with the largest node
      // weight, if both are equal, break ties randomly.
      //
      // This will ensure that the coarse graph has a minimal edge weight, while
      // the node weight distribution is as uniform as possible. By removing the
      // heaviest edges, we will hopefully reduce the edge cut at the coarsest
      // level of the graph partitioning.
      size_t best_neighbor = npos_node;
      weight_t best_edge_weight = 0;
      for (const auto& [fine_neighbor, edge_weight] : fine_graph[fine_node]) {
        if (fine_to_coarse[fine_neighbor] != npos_node) continue;

        if (greater_or(edge_weight,
                       best_edge_weight,
                       less_or(fine_weights[fine_neighbor],
                               fine_weights[best_neighbor],
                               rng))) {
          best_neighbor = fine_neighbor;
          best_edge_weight = edge_weight;
        }
      }
      if (best_neighbor != npos_node) {
        fine_to_coarse[best_neighbor] = coarse_node;
        coarse_to_fine.push_back(best_neighbor);
      }
    }

    // Build the coarse graph.
    impl::build_coarse_graph(fine_graph,
                             fine_weights,
                             coarse_graph,
                             coarse_weights,
                             coarse_to_fine,
                             fine_to_coarse,
                             num_coarse_nodes);
  }

}; // class CoarsenHEM

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Coarse the graph using Sorted Heavy Edge Matching (HEM) algorithm.
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
  /// @param fine_weights   Fine graph node weights.
  /// @param coarse_graph   Coarse graph.
  /// @param coarse_weights Coarse graph node weights.
  /// @param coarse_to_fine Coarse-to-fine mapping.
  /// @param fine_to_coarse Fine-to-coarse mapping.
  template<weighted_graph FineGraph,
           node_weights<FineGraph> FineWeights,
           weighted_graph CoarseGraph,
           node_weights<FineGraph> CoarseWeights,
           node_mapping<CoarseGraph, FineGraph> CoarseToFine,
           node_mapping<FineGraph, CoarseGraph> FineToCoarse>
  static void operator()(const FineGraph& fine_graph,
                         const FineWeights& fine_weights,
                         CoarseGraph& coarse_graph,
                         CoarseWeights& coarse_weights,
                         CoarseToFine& coarse_to_fine,
                         FineToCoarse& fine_to_coarse) {
    TIT_PROFILE_SECTION("Graph::CoarsenGEM::operator()");
    TIT_ASSERT(fine_graph.num_nodes() == fine_weights.size(),
               "Invalid number of the fine graph weights!");

    SplitMix64 rng{fine_graph.num_nodes()};

    // Construct permutation of the fine graph edges.
    //
    // Since the edge weights of the coarse graph are sums of the weights of the
    // corresponding edge weights of the fine graph, we will prioritize the
    // heaviest edges to reduce the minimal edge weight of the coarse graph.
    //
    // Among equally weighted edges, we will prioritize the edges with the
    // smallest node weights to reduce the minimal node weight of the coarse
    // graph, thus making the weight distribution of the coarse graph more
    // uniform.
    //
    // Equally weighted edges will be randomly shuffled to avoid biasing.
    auto fine_edges = fine_graph.edges() | std::ranges::to<std::vector>();
    const auto edge_priority = [&fine_weights](const auto& fe) {
      const auto [edge_weight, fine_neighbor, fine_node] = fe;
      return std::pair{
          edge_weight,
          std::min(fine_weights[fine_neighbor], fine_weights[fine_node])};
    };
    std::ranges::sort(fine_edges, std::greater{}, edge_priority);
    sorted_equality_ranges(
        fine_edges,
        [&rng](std::ranges::view auto fns) { std::ranges::shuffle(fns, rng); },
        std::greater{},
        edge_priority);

    // Build the fine to coarse mapping.
    //
    // Merge pairs of nodes that are connected by an edge, if both nodes are not
    // already mapped to a coarse node. After merging, keep the unmerged nodes
    // as they are.
    size_t num_coarse_nodes = 0;
    fine_to_coarse.assign(fine_graph.num_nodes(), npos_node);
    coarse_to_fine.clear(), coarse_to_fine.reserve(fine_graph.num_nodes());
    for (const auto& [_, fine_node, fine_neighbor] : fine_edges) {
      if (fine_to_coarse[fine_node] != npos_node) continue;
      if (fine_to_coarse[fine_neighbor] != npos_node) continue;

      const auto coarse_node = num_coarse_nodes++;
      fine_to_coarse[fine_node] = coarse_node;
      fine_to_coarse[fine_neighbor] = coarse_node;
      coarse_to_fine.push_back(fine_node);
      coarse_to_fine.push_back(fine_neighbor);
    }
    for (const auto fine_node : fine_graph.nodes()) {
      if (fine_to_coarse[fine_node] != npos_node) continue;

      const auto coarse_node = num_coarse_nodes++;
      fine_to_coarse[fine_node] = coarse_node;
      coarse_to_fine.push_back(fine_node);
    }

    // Build the coarse graph.
    impl::build_coarse_graph(fine_graph,
                             fine_weights,
                             coarse_graph,
                             coarse_weights,
                             coarse_to_fine,
                             fine_to_coarse,
                             num_coarse_nodes);
  }

}; // class CoarsenHEM

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph coarsening function.
template<class Coarsen>
concept coarsen_func = std::same_as<Coarsen, CoarsenGEM> || //
                       std::same_as<Coarsen, CoarsenHEM>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
