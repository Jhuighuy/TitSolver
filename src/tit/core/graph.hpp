/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <tuple>

#include "tit/core/misc.hpp" // IWYU pragma: keep
#include "tit/core/multivector.hpp"
#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Compressed sparse adjacency graph.
\******************************************************************************/
class Graph : public Multivector<size_t> {
public:

  /** Number of graph nodes. */
  constexpr auto num_nodes() const noexcept -> size_t {
    return size();
  }

  /** Range of the unique graph edges. */
  constexpr auto edges() const noexcept {
#if TIT_IWYU
    // Include-what-you-use's clang has no `std::views::join`.
    // Return something with matching type.
    assume_used(this);
    return std::views::single(std::tuple<size_t, size_t>{});
#else
    return std::views::iota(size_t{0}, num_nodes()) |
           std::views::transform([this](size_t row_index) {
             return (*this)[row_index] |
                    // Take only lower part of the row.
                    std::views::take_while([=](size_t col_index) {
                      return col_index < row_index;
                    }) |
                    // Pack row and column indices into a tuple.
                    std::views::transform([=](size_t col_index) {
                      return std::tuple{col_index, row_index};
                    });
           }) |
           std::views::join;
#endif
  }

}; // class Graph

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
