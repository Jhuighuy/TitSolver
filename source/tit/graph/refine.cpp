/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <queue>
#include <ranges>
#include <tuple>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/uint_utils.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/refine.hpp"

namespace tit::graph {

namespace bc = boost::container;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Gain queue element for the Fiduccia-Mattheyses algorithm.
class Gain final {
public:

  // Construct a gain structure.
  constexpr Gain(weight_t gain, size_t node) noexcept
      : gain_{gain}, node_{node} {}

  // Accessors.
  constexpr auto gain() const noexcept -> weight_t {
    return gain_;
  }
  constexpr auto node() const noexcept -> size_t {
    return node_;
  }

  // Compare two elements based on the gain and node index.
  // If the gains are equal, the node index is used as a tie-breaker.
  friend constexpr auto operator<=>(const Gain& a, const Gain& b) noexcept {
    if (const auto cmp = a.gain_ <=> b.gain_; cmp != 0) return cmp;
    return SplitMix64{a.node_}() <=> SplitMix64{b.node_}();
  }

private:

  weight_t gain_;
  size_t node_;

}; // class Gain

// Gain queue for the Fiduccia-Mattheyses algorithm.
class GainQueue final {
public:

  explicit GainQueue(size_t num_nodes) : control_(num_nodes, 0) {}

  [[nodiscard]] auto empty() noexcept -> bool {
    while (!queue_.empty() &&
           queue_.top().gain() != control_[queue_.top().node()]) {
      queue_.pop();
    }
    return queue_.empty();
  }

  void emplace(weight_t gain, size_t node) {
    queue_.emplace(gain, node);
    control_[node] = gain;
  }

  auto pop() noexcept -> size_t {
    TIT_ENSURE(!queue_.empty(), "Gain queue is empty!");
    const auto node = queue_.top().node();
    queue_.pop();
    control_[node] = -999999999999;
    return node;
  }

  auto drop(size_t node) noexcept {
    control_[node] = -999999999999;
  }

private:

  std::priority_queue<Gain> queue_;
  std::vector<weight_t> control_;

}; // class GainQueue

/// Fiduccia-Mattheyses-style graph partition refinement algorithm.
///
/// The algorithm tries to move vertices between partitions to minimize the edge
/// cut while maintaining a balance between partition weights. On each
/// iteration, nodes are moved to partitions that maximize the gain while
/// keeping the balance. Negative gain moves are allowed, the solution is
/// rolled back to the best state achieved during the iteration.
///
/// @param[in]    graph     Graph.
/// @param[in]    weights   Node weights.
/// @param[inout] parts     Node partitioning.
/// @param[in]    num_parts Number of partitions.
/// @param[in]    max_iter  Maximum number of iterations.
/// @param[in]    max_consecutive_negative_moves
///                         Maximum number of consecutive negative moves before
///                         breaking the current iteration.
void refine_parts(const WeightedGraph& graph,
                  const std::vector<weight_t>& weights,
                  std::vector<size_t>& parts,
                  size_t num_parts,
                  size_t max_iter) {
  TIT_PROFILE_SECTION("Graph::refine_parts()");
  TIT_ASSERT(graph.num_nodes() == weights.size(), "Invalid graph weights!");
  TIT_ASSERT(graph.num_nodes() == parts.size(), "Invalid graph parts!");

  // Check if the node is internal to it's current partition.
  const auto is_internal = [&graph, &parts](size_t node) -> bool {
    for (const auto& [neighbor, _] : graph[node]) {
      if (parts[neighbor] != parts[node]) return false;
    }
    return true;
  };

  // Compute the priority for moving a node as a difference between it's
  // external degree (sum weights of edges to nodes in other partitions) and the
  // internal degree (sum weights of edges to nodes in the same partition).
  const auto compute_priority = [&graph, &parts, num_parts](size_t node) {
    bc::small_vector<weight_t, 32> external_degrees(num_parts);
    for (const auto& [neighbor, edge_weight] : graph[node]) {
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
  const auto compute_gain = [&graph, &parts](size_t node,
                                             size_t to_part) -> weight_t {
    weight_t gain = 0;
    const auto from_part = parts[node];
    for (const auto& [neighbor, edge_weight] : graph[node]) {
      if (parts[neighbor] == from_part) gain -= edge_weight;
      else if (parts[neighbor] == to_part) gain += edge_weight;
    }
    return gain;
  };

  // Initialize partition weights distribution and compute the maximum allowed
  // weight disbalance between partitions (~3% of the average part weight).
  weight_t total_weight = 0;
  bc::small_vector<weight_t, 32> part_weights(num_parts, 0);
  for (const auto& [node, weight] : std::views::zip(graph.nodes(), weights)) {
    total_weight += weight;
    part_weights[parts[node]] += weights[node];
  }
  const auto average_weight =
      divide_up(total_weight, static_cast<weight_t>(num_parts));
  const auto max_part_weight = average_weight + divide_up(average_weight, 33);

  bc::small_vector<bool, 32> available_parts(num_parts, false);
  std::vector<bool> moved(graph.num_nodes(), false);
  std::vector<std::tuple<size_t, size_t, size_t>> undo_moves;
  for (size_t iter = 0; iter < max_iter; ++iter) {
    // Build the initial gain queue for the boundary nodes.
    //
    // Since move node priority is different from it's move gain, we do allow
    // negative gains, as they can be beneficial for breaking local minima.
    GainQueue gain_queue{graph.num_nodes()};
    for (const auto node : graph.nodes()) {
      if (is_internal(node)) continue;
      gain_queue.emplace(compute_priority(node), node);
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
    while (!gain_queue.empty() /*&& total_gain >= 0*/) {
      const auto node = gain_queue.pop();

      // Find the available partitions to move the node to.
      std::ranges::fill(available_parts, false);
      for (const auto& [neighbor, _] : graph[node]) {
        if (parts[neighbor] == parts[node]) continue;
        available_parts[parts[neighbor]] = true;
      }

      // Find the best partition to move the node to: the one that respects the
      // weight constraints, maximizes the gain and minimizes the weight
      // disbalance. If no such partition exists, skip the node, it may be
      // revisited later by it's neighbor updates.
      weight_t best_gain = 0;
      size_t best_part = npos_node;
      const auto from_part = parts[node];
      for (size_t test_part = 0; test_part < num_parts; ++test_part) {
        if (!available_parts[test_part]) continue;

        // Check the weight constraints.
        const auto weight = weights[node];
        if (part_weights[test_part] + weight > max_part_weight) continue;

        // Select the best partition to move the node to.
        const auto test_gain = compute_gain(node, test_part);
        if (best_part == npos_node || //
            better(test_gain,
                   best_gain,
                   worse(part_weights[test_part],
                         part_weights[best_part],
                         rng() % 2 == 1))) {
          best_gain = test_gain;
          best_part = test_part;
        }
      }
      if (best_part == npos_node) continue;

      // Move the node to the new partition.
      const size_t to_part = best_part;
      part_weights[from_part] -= weights[node];
      part_weights[to_part] += weights[node];
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
      for (const auto& [neighbor, _] : graph[node]) {
        if (moved[neighbor]) continue;
        gain_queue.drop(neighbor);
        if (is_internal(neighbor)) continue;
        gain_queue.emplace(compute_priority(neighbor), neighbor);
      }
    }

    // Rollback to the best total gain achieved during the iteration.
    for (const auto& [node, from_part, to_part] :
         undo_moves | std::views::reverse) {
      part_weights[to_part] -= weights[node];
      part_weights[from_part] += weights[node];
      parts[node] = from_part;
    }

    // If couldn't achieve a positive total gain, break the iteration.
    if (best_total_gain <= 0) break;
  }
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTNEXTLINE
void GraphPartsRefiner::operator()(const WeightedGraph& graph,
                                   const WeightArray& weights,
                                   PartArray& parts,
                                   size_t num_parts) {
  refine_parts(graph, weights, parts, num_parts, config_.max_iter);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
