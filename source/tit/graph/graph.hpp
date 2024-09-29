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
        nodes() | std::views::transform([this](size_t node) {
          return (*this)[node] |
                 // Take only lower part of the row.
                 std::views::take_while([node](size_t neighbor) { //
                   return neighbor < node;
                 }) |
                 // Pack row and column indices into a tuple.
                 std::views::transform([node](size_t neighbor) {
                   return std::tuple{neighbor, node};
                 });
        }) |
        std::views::join;
  }

}; // class Graph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
