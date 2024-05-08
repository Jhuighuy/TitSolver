/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/multivector.hpp"

namespace tit {

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

}; // class Graph

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
