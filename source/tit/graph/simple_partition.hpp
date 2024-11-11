/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <limits>
#include <numeric>
#include <random>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/rand_utils.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/uint_utils.hpp"

#include "tit/core/utils.hpp"
#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dummy uniform partitioning function.
struct UniformPartition final {
  static void operator()(const auto& graph,
                         const auto& /*weights*/,
                         auto& parts,
                         size_t num_parts) {
    const auto num_nodes = graph.num_nodes();
    const auto part_size = num_nodes / num_parts;
    const auto remainder = num_nodes % num_parts;
    for (size_t part = 0; part < num_parts; ++part) {
      const auto first = part * part_size + std::min(part, remainder);
      const auto last = (part + 1) * part_size + std::min(part + 1, remainder);
      for (size_t i = first; i < last; ++i) parts[i] = part;
    }
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct SimplePartition final {
  static void operator()(const auto& graph,
                         const auto& weights,
                         auto& parts,
                         size_t num_parts) {
    TIT_PROFILE_SECTION("UniformPartition::operator()");
    std::mt19937_64 rng{/*seed=*/graph.num_nodes()};

    // Identify the connected components.
    std::vector<part_t> components(graph.num_nodes(), npos);
    size_t num_components = 0;
    std::vector<weight_t> component_weights{};
    for (size_t done = 0; done < graph.num_nodes();) {
      // Find the first node with the unassigned component.
      const auto first_node = *std::ranges::find_if( //
          graph.nodes(),
          [&components](node_t node) { return components[node] == npos; });
      const auto component = num_components++;
      components[first_node] = component;
      component_weights.push_back(weights[first_node]);
      done += 1;

      // Breadth-first search to find the connected component.
      while (true) {
        bool any_change = false;
        for (const auto node : graph.nodes()) {
          if (components[node] != component) continue;
          for (const auto& [neighbor, _] : graph[node]) {
            auto& neighbor_component = components[neighbor];
            if (neighbor_component == component) continue;
            TIT_ENSURE(neighbor_component == npos,
                       "Neighbor component is already assigned!");
            neighbor_component = component;
            component_weights.back() += weights[neighbor];
            any_change = true;
            done += 1;
          }
        }
        if (!any_change) break;
      }
    }

    // Calculate the total weight of the graph end the maximum part weight.
    const auto total_weight =
        std::reduce(component_weights.begin(), component_weights.end());
    const auto average_part_weight =
        divide_up(total_weight, static_cast<weight_t>(num_parts));
    const auto part_weight_cap = average_part_weight - average_part_weight / 33;

    // Sort the components by the component weights.
    auto component_perm =
        iota_perm(component_weights) | std::ranges::to<std::vector>();
    std::ranges::sort( //
        component_perm,
        /*cmp=*/{},
        [&component_weights](size_t c) { return component_weights[c]; });

    // Partition the components from the smallest to the largest.
    std::ranges::fill(parts, npos);
    std::vector<weight_t> part_weights(num_parts);
    for (const auto component : component_perm) {
      const auto iter_component_nodes = [&graph, &components, component] {
        return graph.nodes() |
               std::views::filter([&components, component](node_t node) {
                 return components[node] == component;
               });
      };

      // Do a greedy partitioning.
      for (size_t part = 0;;) {
        if (part_weights[part] >= part_weight_cap) {
          // Find the part with the smallest weight.
          for (size_t test_part = 0; test_part < num_parts; ++test_part) {
            if (part_weights[test_part] < part_weights[part]) part = test_part;
          }
        }

        // Find the node with largest gain. Min Max Greedy algorithm.
        node_t best_node = npos;
        std::tuple best_gain{std::numeric_limits<weight_t>::min(),
                             std::numeric_limits<weight_t>::min()};
        for (const auto node : iter_component_nodes()) {
          if (parts[node] != npos) continue;

          // Compute the gain.
          weight_t internal_degree = 0;
          weight_t external_degree = 0;
          for (const auto& [neighbor, edge_weight] : graph[node]) {
            const auto neighbor_part = parts[neighbor];
            if (neighbor_part == npos) continue;
            if (neighbor_part == part) internal_degree += edge_weight;
            else external_degree += edge_weight;
          }

          const std::tuple gain{weight_t{internal_degree > 0},
                                external_degree > 0 ? -external_degree :
                                                      internal_degree};

          // Update the best node and gain.
          if (greater_or(gain,
                         best_gain,
                         less_or(weights[node], weights[best_node], rng))) {
            best_gain = gain;
            best_node = node;
          }
        }
        if (best_node == npos) break;

        // Select the random node from the best nodes.
        const auto node = best_node;
        parts[node] = part;
        part_weights[part] += weights[node];
      }
    }

    std::ranges::sort(part_weights);
    TIT_STATS("disbalance",
              static_cast<double>(part_weights.back() - part_weights.front()) /
                  static_cast<double>(part_weights.back()));
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
