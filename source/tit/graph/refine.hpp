/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/containers/priority_queue.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand_utils.hpp"
#include "tit/core/uint_utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fiduccia-Mattheyses-style graph partition refinement.
///
/// The graph partition refinement is used to refine the partitioning of the
/// given graph. The refinement is done by moving nodes between partitions to
/// minimize the edge cut while maintaining a balance between partition weights.
///
/// On each iteration, nodes are moved to partitions that maximize the gain
/// while keeping the balance. Negative gain moves are allowed, the solution is
/// rolled back to the best state achieved during the iteration.
class RefinePartsFM final {
public:

  /// Initialize the graph partition refiner.
  ///
  /// @param max_disbalance
  ///   Maximum allowed disbalance between partition weights in percents.
  /// @param max_iter
  ///   Maximum number of iterations.
  constexpr explicit RefinePartsFM(weight_t max_disbalance = 3,
                                   size_t max_iter = 20)
      : max_disbalance_{max_disbalance}, max_iter_{max_iter} {}

  /// Refine the partitioning of the given graph.
  template<weighted_graph Graph, node_parts<Graph> Parts>
  void operator()(const Graph& graph, Parts& parts, size_t num_parts) const {
    TIT_PROFILE_SECTION("Graph::RefinePartsFM::operator()");
    TIT_ASSERT(graph.num_nodes() == parts.size(), "Invalid graph parts!");

    // Check if the node is internal to it's current partition.
    const auto is_internal = [&graph, &parts](size_t node) -> bool {
      for (const auto& neighbor : graph.edges(node)) {
        if (parts[neighbor] != parts[node]) return false;
      }
      return true;
    };

    // Compute the priority for moving a node as a difference between it's
    // external degree (sum weights of edges to nodes in other partitions) and
    // the internal degree (sum weights of edges to nodes in the same
    // partition).
    const auto compute_priority = [&graph, &parts, num_parts](size_t node) {
      SmallVector<weight_t, 32> external_degrees(num_parts);
      for (const auto& [neighbor, edge_weight] : graph.wedges(node)) {
        const auto neighbor_part = parts[neighbor];
        external_degrees[neighbor_part] += edge_weight;
      }
      weight_t max_external_degree = 0;
      for (size_t part = 0; part < num_parts; ++part) {
        if (part == parts[node]) continue;
        max_external_degree =
            std::max(max_external_degree, external_degrees[part]);
      }
      const auto internal_degree = external_degrees[parts[node]];
      const auto priority = max_external_degree - internal_degree;
      return priority;
    };

    // Compute the gain or loss for moving a node from it's current part to
    // the specified part.
    //
    // Here, as before, we approximate the edge weights by the product of the
    // weights of the nodes that are connected by the edge.
    const auto compute_gain = [&graph, &parts](node_t node,
                                               part_t to_part) -> weight_t {
      weight_t gain = 0;
      const auto from_part = parts[node];
      for (const auto& [neighbor, edge_weight] : graph.wedges(node)) {
        if (parts[neighbor] == from_part) gain -= edge_weight;
        else if (parts[neighbor] == to_part) gain += edge_weight;
      }
      return gain;
    };

    // Initialize partition weights distribution and compute the maximum allowed
    // weight disbalance between partitions (~3% of the average part weight).
    weight_t total_weight = 0;
    SmallVector<weight_t, 32> part_weights(num_parts, 0);
    for (const auto& [node, weight] : graph.wnodes()) {
      total_weight += weight;
      part_weights[parts[node]] += weight;
    }
    const weight_t average_weight =
        to_signed(divide_up(to_unsigned(total_weight), num_parts));
    const weight_t divisor = 100 / max_disbalance_;
    const weight_t max_part_weight =
        average_weight +
        to_signed(divide_up(to_unsigned(average_weight), to_unsigned(divisor)));

    SmallVector<bool, 32> available_parts(num_parts, false);
    std::vector<bool> moved(graph.num_nodes(), false);
    std::vector<std::tuple<size_t, size_t, size_t>> undo_moves;
    for (size_t iter = 0; iter < max_iter_; ++iter) {
      // Build the initial gain queue for the boundary nodes.
      //
      // Since move node priority is different from it's move gain, we do allow
      // negative gains, as they can be beneficial for breaking local minima.
      KeyedPriorityQueue<weight_t> gain_queue{graph.num_nodes()};
      for (const auto node : graph.nodes()) {
        if (is_internal(node)) continue;
        gain_queue.emplace(node, compute_priority(node));
      }

      // Try to move nodes between partitions based on gain and balance.
      // Once a node is moved, it's neighbors are re-evaluated for potential
      // moves, and the node is not considered for further moves in this
      // iteration.
      //
      // We'll continue moving nodes until the total gain becomes negative, or
      // until we've made too many negative moves at once.
      //
      // We'll keep track of the best total gain achieved during the iteration.
      // Once the iteration is complete, we'll rollback to the best total gain
      // achieved during the iteration.
      weight_t total_gain = 0;
      weight_t best_total_gain = 0;
      std::ranges::fill(moved, false);
      undo_moves.clear();
      SplitMix64 rng{/*seed=*/graph.num_nodes()};
      while (!gain_queue.empty()) {
        const auto& [node, _] = gain_queue.pop();

        // Find the available partitions to move the node to.
        std::ranges::fill(available_parts, false);
        for (const auto& neighbor : graph.edges(node)) {
          if (parts[neighbor] == parts[node]) continue;
          available_parts[parts[neighbor]] = true;
        }

        // Find the best partition to move the node to: the one that respects
        // the weight constraints, maximizes the gain and minimizes the weight
        // disbalance. If no such partition exists, skip the node, it may be
        // revisited later by it's neighbor updates.
        weight_t best_gain = 0;
        part_t best_part = npos;
        const auto from_part = parts[node];
        for (size_t test_part = 0; test_part < num_parts; ++test_part) {
          if (!available_parts[test_part]) continue;

          // Check the weight constraints.
          const auto weight = graph.weight(node);
          if (part_weights[test_part] + weight > max_part_weight) continue;

          // Select the best partition to move the node to.
          const auto test_gain = compute_gain(node, test_part);
          if (best_part == npos || greater_or(test_gain,
                                              best_gain,
                                              less_or(part_weights[test_part],
                                                      part_weights[best_part],
                                                      rng))) {
            best_gain = test_gain;
            best_part = test_part;
          }
        }
        if (best_part == npos) continue;

        // Move the node to the new partition.
        const size_t to_part = best_part;
        part_weights[from_part] -= graph.weight(node);
        part_weights[to_part] += graph.weight(node);
        parts[node] = to_part;
        moved[node] = true;

        // Update the total gain and best total gain.
        total_gain += best_gain;
        if (total_gain > best_total_gain) {
          best_total_gain = total_gain;
          undo_moves.clear();
        } else {
          undo_moves.emplace_back(node, from_part, to_part);
        }

        // Recompute gains for the neighbors.
        for (const auto& neighbor : graph.edges(node)) {
          if (moved[neighbor]) continue;
          gain_queue.erase(neighbor);
          if (is_internal(neighbor)) continue;
          gain_queue.emplace(neighbor, compute_priority(neighbor));
        }
      }

      // Rollback to the best total gain achieved during the iteration.
      for (const auto& [node, from_part, to_part] :
           undo_moves | std::views::reverse) {
        part_weights[to_part] -= graph.weight(node);
        part_weights[from_part] += graph.weight(node);
        parts[node] = from_part;
      }

      // If couldn't achieve a positive total gain, break the iteration.
      if (best_total_gain <= 0) break;
    }
  }

private:

  weight_t max_disbalance_;
  size_t max_iter_;

}; // class RefinePartsFM

/// Feduccia-Mattheyses-style graph partition refinement.
inline constexpr RefinePartsFM refine_parts_fm{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning refinement function.
template<class Refine>
concept refine_func = std::same_as<Refine, RefinePartsFM>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
