/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <deque>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Breadth-first search.
///
/// @param graph     Graph to search in.
/// @param seed_node Starting node.
/// @param pred      Node predicate.
/// @param func      Visited node callback. Once the callback returns `false`,
///                  the search is stopped.
template<graph Graph,
         node_predicate Pred = AlwaysTrue,
         std::invocable<node_t> Func>
  requires std::same_as<std::invoke_result_t<Func, node_t>, bool>
constexpr auto bfs(const Graph& graph, node_t seed_node, Pred pred, Func func) {
  TIT_ASSERT(seed_node < graph.num_nodes(), "Seed node is out of range!");
  TIT_ASSERT(std::invoke(pred, seed_node), "Seed node is not in the graph!");
  if (!std::invoke(func, seed_node)) return;
  std::vector<bool> visited(graph.num_nodes(), false);
  visited[seed_node] = true;
  for (std::deque<node_t> frontier{seed_node}; !frontier.empty();) {
    const auto node = frontier.front();
    frontier.pop_front();
    for (const auto neighbor : graph.edges(node)) {
      if (!std::invoke(pred, neighbor) || visited[neighbor]) continue;
      if (!std::invoke(func, neighbor)) return;
      frontier.push_back(neighbor);
      visited[neighbor] = true;
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Find connected components of the given graph.
template<graph Graph,
         node_parts<Graph> Components,
         node_predicate Pred = AlwaysTrue>
constexpr auto connected_components(const Graph& graph,
                                    Components&& components,
                                    Pred pred = {}) -> size_t {
  TIT_ASSUME_UNIVERSAL(Components, components);
  if constexpr (std::ranges::sized_range<Components>) {
    TIT_ASSERT(std::size(components) == graph.num_nodes(),
               "Invalid number of components!");
  }

  // Initialize the components.
  std::ranges::fill(components, npos);

  // Find the connected components.
  for (part_t component = 0;; ++component) {
    // Find the seed node with the unassigned component.
    const auto iter = std::ranges::find_if( //
        graph.nodes(),
        [&pred, &components](node_t node) {
          return std::invoke(pred, node) && components[node] == npos;
        });
    if (iter == std::end(graph.nodes())) return component;

    // Walk through the nodes in a BFS fashion and assign the component.
    bfs(graph, *iter, std::ref(pred), [&components, component](node_t node) {
      TIT_ASSERT(components[node] == npos || components[node] == component,
                 "Component of the node is already assigned!");
      components[node] = component;
      return true;
    });
  }
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the edge cut of a graph partitioning.
///
/// The edge cut is the sum of the weights of the edges that connect nodes from
/// different partitions.
template<graph Graph, node_parts<Graph> Parts>
constexpr auto edge_cut(const Graph& graph, Parts&& parts) noexcept
    -> weight_t {
  TIT_ASSUME_UNIVERSAL(Parts, parts);
  weight_t edge_cut = 0;
  for (const auto& [node, neighbor, edge_weight] : graph.wedges()) {
    if (parts[node] != parts[neighbor]) edge_cut += edge_weight;
  }
  return edge_cut * 2;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
