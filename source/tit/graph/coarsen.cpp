/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <functional>
#include <ranges>
#include <utility>
#include <vector>

#include <boost/container/container_fwd.hpp>
#include <boost/container/flat_map.hpp>     // IWYU pragma: keep
#include <boost/container/small_vector.hpp> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/coarsen.hpp"
#include "tit/graph/graph.hpp"

namespace tit::graph {

namespace bc = boost::container;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GraphCoarsener::GraphCoarsener(const WeightedGraph& fine_graph,
                               const WeightArray& fine_weights,
                               const CoarseningConfig& config)
    : config_{config}, rng_{/*seed=*/fine_graph.num_nodes()} {
  TIT_PROFILE_SECTION("GraphCoarsener::GraphCoarsener()");
  TIT_ASSERT(fine_graph.num_nodes() == fine_weights.size(),
             "Invalid number of the fine graph weights!");

  // Build the fine-to-coarse mapping using the specified strategy.
  const auto num_coarse_nodes = [&] {
    switch (config_.strategy) {
      case CoarseningStrategy::heavy_edge_matching:
        return match_nodes_hem_(fine_graph, fine_weights);
        break;
      case CoarseningStrategy::greedy_edge_matching:
        return match_nodes_gem_(fine_graph, fine_weights);
        break;
      default: TIT_FAIL("Invalid coarsening strategy!");
    }
  }();

  // Build the coarse graph.
  build_graph_(fine_graph, fine_weights, num_coarse_nodes);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto GraphCoarsener::match_nodes_hem_(const WeightedGraph& fine_graph,
                                      const WeightArray& fine_weights)
    -> size_t {
  // Construct permutation of the fine graph nodes.
  //
  // Since the node weights of the coarse graph are sums of the weights of the
  // corresponding node weights of the fine graph, we will prioritize the least
  // weigted nodes to reduce the minimal weight of the coarse graph, thus
  // making the weight distribution of the coarse graph more uniform.
  //
  // Equally weighted nodes will be randomly shuffled to avoid biasing.
  auto fine_nodes = fine_graph.nodes() | std::ranges::to<std::vector>();
  const auto node_priority = [&fine_weights](size_t fn) {
    return fine_weights[fn];
  };
  std::ranges::sort(fine_nodes, std::less{}, node_priority);
  seq::equal_subranges(
      fine_nodes,
      [this](std::ranges::view auto fns) { std::ranges::shuffle(fns, rng_); },
      /*pred=*/{},
      /*proj=*/node_priority);

  // Build the fine to coarse mapping.
  size_t num_coarse_nodes = 0;
  fine_to_coarse_.assign(fine_graph.num_nodes(), npos_node);
  coarse_to_fine_.clear(), coarse_to_fine_.reserve(fine_graph.num_nodes());
  for (const auto fine_node : fine_nodes) {
    if (fine_to_coarse_[fine_node] != npos_node) continue;

    // Map the fine node to a new coarse node.
    const auto coarse_node = num_coarse_nodes++;
    fine_to_coarse_[fine_node] = coarse_node;
    coarse_to_fine_.push_back(fine_node);

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
      if (fine_to_coarse_[fine_neighbor] != npos_node) continue;

      if (better(edge_weight,
                 best_edge_weight,
                 better(fine_weights[fine_neighbor],
                        fine_weights[best_neighbor],
                        rng_() % 2 == 1))) {
        best_neighbor = fine_neighbor;
        best_edge_weight = edge_weight;
      }
    }
    if (best_neighbor != npos_node) {
      fine_to_coarse_[best_neighbor] = coarse_node;
      coarse_to_fine_.push_back(best_neighbor);
    }
  }

  return num_coarse_nodes;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto GraphCoarsener::match_nodes_gem_(const WeightedGraph& fine_graph,
                                      const WeightArray& fine_weights)
    -> size_t {
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
    /// @todo Is `std::min` the correct choice here?
    return std::pair{
        edge_weight,
        std::min(fine_weights[fine_neighbor], fine_weights[fine_node])};
  };
  std::ranges::sort(fine_edges, std::greater{}, edge_priority);
  seq::equal_subranges(
      fine_edges,
      [this](std::ranges::view auto fns) { std::ranges::shuffle(fns, rng_); },
      /*pred=*/{},
      /*proj=*/edge_priority);

  // Build the fine to coarse mapping.
  //
  // Merge pairs of nodes that are connected by an edge, if both nodes are not
  // already mapped to a coarse node. After merging, keep the unmerged nodes
  // as they are.
  size_t num_coarse_nodes = 0;
  fine_to_coarse_.assign(fine_graph.num_nodes(), npos_node);
  coarse_to_fine_.clear(), coarse_to_fine_.reserve(fine_graph.num_nodes());
  for (const auto& [_, fine_node, fine_neighbor] : fine_edges) {
    if (fine_to_coarse_[fine_node] != npos_node) continue;
    if (fine_to_coarse_[fine_neighbor] != npos_node) continue;

    const auto coarse_node = num_coarse_nodes++;
    fine_to_coarse_[fine_node] = coarse_node;
    fine_to_coarse_[fine_neighbor] = coarse_node;
    coarse_to_fine_.push_back(fine_node);
    coarse_to_fine_.push_back(fine_neighbor);
  }
  for (const auto fine_node : fine_graph.nodes()) {
    if (fine_to_coarse_[fine_node] != npos_node) continue;

    const auto coarse_node = num_coarse_nodes++;
    fine_to_coarse_[fine_node] = coarse_node;
    coarse_to_fine_.push_back(fine_node);
  }

  return num_coarse_nodes;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void GraphCoarsener::build_graph_(const WeightedGraph& fine_graph,
                                  const WeightArray& fine_weights,
                                  size_t num_coarse_nodes) {
  // Reserve space for the coarse graph.
  coarse_weights_.clear(), coarse_weights_.reserve(num_coarse_nodes);
  coarse_graph_.clear(), coarse_graph_.reserve(num_coarse_nodes);

  // Build the coarse graph.
  seq::equal_subranges(
      coarse_to_fine_,
      [this, &fine_graph, &fine_weights](std::ranges::view auto fine_nodes) {
        bc::small_flat_map<size_t, weight_t, 32> coarse_neighbors{};
        weight_t coarse_weight = 0;
        for (const auto fine_node : fine_nodes) {
          for (const auto& [fine_neighbor, //
                            fine_edge_weight] : fine_graph[fine_node]) {
            const auto coarse_neighbor = fine_to_coarse_[fine_neighbor];
            coarse_neighbors[coarse_neighbor] += fine_edge_weight;
          }
          coarse_weight += fine_weights[fine_node];
        }
        coarse_graph_.append_bucket(coarse_neighbors);
        coarse_weights_.push_back(coarse_weight);
      },
      /*pred=*/{},
      /*proj=*/[this](size_t fn) { return fine_to_coarse_[fn]; });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
