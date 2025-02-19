/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand_utils.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/refine.hpp"
#include "tit/graph/utils.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Greedy graph partitioning function.
///
/// @todo Write an explanation of what is going on here.
class GreedyPartition final {
public:

  /// Partition the graph using the greedy partitioning algorithm.
  template<weighted_graph Graph, node_parts<Graph> Parts>
  void operator()(const Graph& graph, Parts&& parts, size_t num_parts) const {
    TIT_PROFILE_SECTION("Graph::GreedyPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(num_parts <= graph.num_nodes(),
               "Number of nodes cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(parts) == graph.num_nodes(),
                 "Size of parts range must be equal to the number of nodes!");
    }

    SplitMix64 rng{graph.num_nodes()};

    // Calculate the total weight of the graph end the maximum part weight.
    auto total_weight =
        std::reduce(graph.weights().begin(), graph.weights().end());
    size_t num_full_parts = 0;
    auto average_part_weight = total_weight / static_cast<weight_t>(num_parts);
    auto part_weight_cap = average_part_weight;

    // Initialize the partitioning.
    std::ranges::fill(parts, npos);
    std::vector<weight_t> part_weights(num_parts);

    size_t part = 0;
    while (true) {
      // Identify the connected components. If there are no components, break.
      std::vector<part_t> components(graph.num_nodes());
      const size_t num_components = connected_components( //
          graph,
          components,
          [&parts](node_t node) { return parts[node] == npos; });
      if (num_components == 0) break;

      // Calculate the component weights.
      std::vector<weight_t> component_weights(num_components, 0);
      for (const auto& [node, weight] : graph.wnodes()) {
        const auto component = components[node];
        if (component == npos) continue;
        component_weights[component] += weight;
      }

      // Find the component with the smallest weight.
      const auto iter = std::ranges::min_element(component_weights);
      const auto component =
          static_cast<size_t>(iter - component_weights.begin());

      // Partition the current components.
      const auto iter_component_nodes = [&graph, &components, component] {
        return graph.nodes() |
               std::views::filter([&components, component](node_t node) {
                 return components[node] == component;
               });
      };

      // Find the part with the smallest weight.
      if (part_weights[part] >= part_weight_cap) {
        num_full_parts += 1;
        total_weight -= part_weights[part];
        average_part_weight = total_weight / (num_parts - num_full_parts);
        part_weight_cap = average_part_weight;
        part += 1;
      }

      // Find the node seed node with the smallest weight.
      node_t seed_node = npos;
      weight_t seed_gain = std::numeric_limits<weight_t>::min();
      weight_t seed_weight = std::numeric_limits<weight_t>::max();
      for (const auto node : iter_component_nodes()) {
        if (parts[node] != npos) continue;

        weight_t gain = 0;
        for (const auto& [neighbor, edge_weight] : graph.wedges(node)) {
          if (parts[neighbor] == npos) gain -= 1;
          else gain += 1;
        }

        const auto weight = graph.weight(node);
        if (greater_or(gain, seed_gain, less_or(weight, seed_weight, rng))) {
          seed_gain = gain;
          seed_node = node;
          seed_weight = weight;
        }
      }
      if (seed_node == npos) break;

      // Grow the partition in a BFS fashion.
      parts[seed_node] = part;
      part_weights[part] += seed_weight;
      std::vector<node_t> frontier{seed_node};
      while (!frontier.empty() && part_weights[part] < part_weight_cap) {
        const auto node = frontier.front();
        frontier.erase(frontier.begin());
        const auto frontier_old_size = frontier.size();
        for (const auto& neighbor : graph.edges(node)) {
          if (parts[neighbor] != npos) continue;
          frontier.push_back(neighbor);

          parts[neighbor] = part;
          part_weights[part] += graph.weight(neighbor);
          if (part_weights[part] >= part_weight_cap) break;
        }
        std::ranges::sort( //
            frontier.begin() + static_cast<ssize_t>(frontier_old_size),
            frontier.end(),
            std::greater{},
            [part, &graph, &parts](node_t nn) {
              weight_t internal_degree = 0;
              weight_t external_degree = 0;
              for (const auto& [neighbor, edge_weight] : graph.wedges(nn)) {
                const auto neighbor_part = parts[neighbor];
                if (neighbor_part == part) internal_degree += edge_weight;
                else external_degree += edge_weight;
              }
              return std::tuple{
                  // Differential Graph.
                  internal_degree - external_degree,
                  // Prioritize the nodes with the smallest weight.
                  -graph.weight(nn),
              };
            });
      }
    }

    refine_parts_fm(graph, parts, num_parts);
    TIT_STATS("edge_cut", edge_cut(graph, parts));

    std::ranges::sort(part_weights);
    TIT_STATS("disbalance",
              static_cast<double>(part_weights.back() - part_weights.front()) /
                  static_cast<double>(part_weights.back()));
  }

}; // class GreedyPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
