/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp" // IWYU pragma: keep
#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Compressed sparse adjacency graph.
\******************************************************************************/
class Graph {
private:

  std::vector<size_t> _row_addrs{0};
  std::vector<size_t> _col_indices;

public:

  /** Number of graph rows. */
  constexpr auto num_rows() const noexcept -> size_t {
    return _row_addrs.size() - 1;
  }

  /** Clear the graph. */
  constexpr void clear() noexcept {
    _row_addrs.clear(), _row_addrs.push_back(0);
    _col_indices.clear();
  }

  /** Append a row to the graph. */
  template<std::ranges::input_range ColIndices>
    requires std::same_as<std::ranges::range_value_t<ColIndices>, size_t>
  constexpr void append_row(ColIndices&& col_indices) {
    std::ranges::copy(col_indices, std::back_inserter(_col_indices));
    std::sort(_col_indices.begin() + _row_addrs.back(), _col_indices.end());
    _row_addrs.push_back(_col_indices.size());
  }

  /** Column indices for the specified row. */
  constexpr auto operator[](size_t row_index) const noexcept {
    TIT_ASSERT(row_index < num_rows(), "Row index is out of range.");
    return std::ranges::subrange{
        _col_indices.cbegin() + _row_addrs[row_index], //
        _col_indices.cbegin() + _row_addrs[row_index + 1]};
  }

  /** Range of the unique graph edges. */
  constexpr auto edges() const noexcept {
#if TIT_IWYU
    // Include-what-you-use's clang has no `std::views::join`.
    // Return something with matching type.
    return std::views::single(std::tuple<size_t, size_t>{});
#else
    return std::views::iota(size_t{0}, num_rows()) |
           std::views::transform([this](size_t row_index) {
             return (*this)[row_index] |
                    // Take only lower part of the row.
                    std::views::take_while([=](size_t col_index) {
                      return col_index <= row_index;
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
