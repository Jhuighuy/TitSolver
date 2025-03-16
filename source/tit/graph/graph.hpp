/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/multivector.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node type alias.
using node_t = size_t;

/// Weight type alias.
using weight_t = ssize_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph.
class Graph : public Multivector<size_t> {
public:

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return size();
  }

  /// Range of the unique graph edges.
  constexpr auto edges() const noexcept {
    return std::views::iota(0UZ, num_nodes()) |
           std::views::transform([this](size_t row_index) {
             return (*this)[row_index] |
                    // Take only lower part of the row.
                    std::views::take_while([row_index](size_t col_index) {
                      return col_index < row_index;
                    }) |
                    // Pack row and column indices into a tuple.
                    std::views::transform([row_index](size_t col_index) {
                      return std::tuple{col_index, row_index};
                    });
           }) |
           std::views::join;
  }

  template<class Func>
  constexpr auto transform_edges(Func fn) const noexcept {
    return std::views::iota(0UZ, num_nodes()) |
           std::views::transform([this, fn](size_t row_index) {
             return (*this)[row_index] |
                    // Take only lower part of the row.
                    std::views::take_while([row_index](size_t col_index) {
                      return col_index < row_index;
                    }) |
                    // Pack row and column indices into a tuple.
                    std::views::transform([row_index](size_t col_index) {
                      return std::tuple{col_index, row_index};
                    }) |
                    // Apply the transformation function.
                    std::views::transform(fn);
           }) |
           std::views::join;
  }

}; // class Graph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph with edge weights.
template<class Base>
class BaseWeightedGraph final : public Base {
public:

  using Base::Base;

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
                 std::views::take_while(
                     [node](const auto& neighbor_and_weight) {
                       const auto& neighbor = std::get<0>(neighbor_and_weight);
                       return neighbor < node;
                     }) |
                 std::views::transform([node](const auto& neighbor_and_weight) {
                   const auto& [neighbor, weight] = neighbor_and_weight;
                   return std::tuple{weight, neighbor, node};
                 });
        }) |
        std::views::join;
  }

}; // class BaseWeightedGraph

/// Alias for a weighted graph using a multivector container.
using WeightedGraph =
    BaseWeightedGraph<Multivector<std::tuple<node_t, weight_t>>>;

/// Alias for a weighted graph with a capped number of edges.
template<size_t MaxNumEdges>
using CapWeightedGraph = BaseWeightedGraph<
    CapMultivector<std::tuple<node_t, weight_t>, MaxNumEdges>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
