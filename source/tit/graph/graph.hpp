/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <limits>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/multivector.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node type alias.
using node_t = size_t;

/// Part type alias.
using part_t = size_t;

/// Weight type alias.
using weight_t = ssize_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node index that is not present in the graph.
constexpr auto npos_node = std::numeric_limits<node_t>::max();

/// Part index that is not present in the graph.
constexpr auto npos_part = std::numeric_limits<part_t>::max();

/// Weight index that is not present in the graph.
constexpr auto npos_weight = std::numeric_limits<weight_t>::min();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node mapping type alias.
using NodeMapping = std::vector<node_t>;

/// Inverse node mapping type alias.
using InverseNodeMapping = std::vector<node_t>;

/// Part array type alias.
using PartArray = std::vector<part_t>;

/// Weight array type alias.
using WeightArray = std::vector<weight_t>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph.
class Graph final : public Multivector<size_t> {
public:

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return size();
  }

  /// Range of the graph nodes.
  /// @{
  constexpr auto nodes() const noexcept {
    return std::views::iota(node_t{0}, node_t{num_nodes()});
  }
  template<class Func>
  constexpr auto transform_nodes(Func func) const {
    return nodes() | std::views::transform(std::move(func));
  }
  /// @}

  /// Range of the unique graph edges.
  /// @{
  constexpr auto edges() const noexcept {
    return transform_nodes([this](node_t node) {
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
  template<class Func>
  constexpr auto transform_edges(Func func) const {
    return transform_nodes([func = std::move(func), this](node_t node) {
             return (*this)[node] |
                    std::views::take_while([node](node_t neighbor) { //
                      return neighbor < node;
                    }) |
                    std::views::transform([node](node_t neighbor) {
                      return std::tuple{neighbor, node};
                    }) |
                    std::views::transform(std::ref(func));
           }) |
           std::views::join;
  }
  /// @}

}; // class Graph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph with edge weights.
template<class Base>
class BaseWeightedGraph final : public Base {
public:

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return Base::size();
  }

  /// Range of the graph nodes.
  constexpr auto nodes() const noexcept {
    return std::views::iota(node_t{0}, node_t{num_nodes()});
  }

  /// Range of the unique graph weighted edges.
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
                   return std::tuple{weight, neighbor, node};
                 });
        }) |
        std::views::join;
  }

}; // class WeightedGraph

using WeightedGraph =
    BaseWeightedGraph<Multivector<std::tuple<size_t, weight_t>>>;

template<size_t MaxNumEdges>
using SmallWeightedGraph = BaseWeightedGraph<
    SmallMultivector<std::tuple<size_t, weight_t>, MaxNumEdges>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
