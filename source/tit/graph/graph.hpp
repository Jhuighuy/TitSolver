/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/multivector.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node type alias.
using node_t = size_t;

/// Weight type alias.
using weight_t = real_t;

/// Part type alias.
using part_t = size_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph.
class Graph final : public Multivector<size_t> {
public:

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return size();
  }

  /// Range of the graph nodes.
  constexpr auto nodes() const noexcept {
    return std::views::iota(size_t{0}, num_nodes());
  }

  /// Range of the unique graph edges.
  constexpr auto edges() const noexcept {
    return //
        nodes() | std::views::transform([this](node_t node) {
          return (*this)[node] |
                 std::views::take_while([node](node_t neighbor) { //
                   return neighbor < node;
                 }) |
                 std::views::transform([node](node_t neighbor) {
                   return std::tuple{neighbor, node};
                 });
        }) |
        std::views::join;
  }

}; // class Graph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph with edge weights.
class WeightedGraph final : public Multivector<std::pair<size_t, weight_t>> {
public:

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return size();
  }

  /// Range of the graph nodes.
  constexpr auto nodes() const noexcept {
    return std::views::iota(size_t{0}, num_nodes());
  }

  /// Range of the unique graph wieghted edges.
  constexpr auto edges() const noexcept {
    return //
        nodes() | std::views::transform([this](node_t node) {
          return (*this)[node] |
                 std::views::take_while([node](const auto& neigbor_and_weight) {
                   const auto& neighbor = std::get<0>(neigbor_and_weight);
                   return neighbor < node;
                 }) |
                 std::views::transform([node](const auto& neigbor_and_weight) {
                   const auto& [neighbor, weight] = neigbor_and_weight;
                   return std::tuple{neighbor, node, weight};
                 });
        }) |
        std::views::join;
  }

}; // class WeightedGraph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
