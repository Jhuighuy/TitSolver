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
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <tuple>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/par.hpp"
#include "tit/core/types.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Compressed vector that can handle multiple elements at a single position.
\******************************************************************************/
template<class Val>
class Multivector {
private:

  // These two zeroes are intentional!
  std::vector<size_t> val_ranges_{0};
  std::vector<Val> vals_;

public:

  /** Multivector size. */
  constexpr auto size() const noexcept -> size_t {
    return val_ranges_.size() - 1;
  }
  /** Is multivector empty? */
  constexpr auto empty() const noexcept -> bool {
    return val_ranges_.size() == 1;
  }

  /** Clear the multivector. */
  constexpr void clear() noexcept {
    val_ranges_.clear(), val_ranges_.push_back(0);
    vals_.clear();
  }

  /** Range of values at index. */
  /** @{ */
  constexpr auto operator[](size_t index) noexcept {
    TIT_ASSERT(index < size(), "Multivector index is out of range.");
    return std::ranges::subrange{vals_.begin() + val_ranges_[index],
                                 vals_.begin() + val_ranges_[index + 1]};
  }
  constexpr auto operator[](size_t index) const noexcept {
    TIT_ASSERT(index < size(), "Multivector index is out of range.");
    return std::ranges::subrange{vals_.begin() + val_ranges_[index],
                                 vals_.begin() + val_ranges_[index + 1]};
  }
  /** @} */

  /** Append values to the multivector. */
  template<std::ranges::input_range Vals>
    requires std::indirectly_copyable<std::ranges::iterator_t<Vals>,
                                      std::ranges::iterator_t<std::vector<Val>>>
  constexpr void push_back(Vals&& vals) {
    std::ranges::copy(vals, std::back_inserter(vals_));
    val_ranges_.push_back(vals_.size());
  }

  /** Sort values of each index. */
  template<class Cmp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<std::ranges::iterator_t<std::vector<Val>>, Cmp, Proj>
  constexpr void sort(Cmp cmp = {}, Proj proj = {}) noexcept {
    par::for_each(std::views::iota(size_t{0}, size()), [=, this](size_t index) {
      std::ranges::sort((*this)[index], cmp, proj);
    });
  }

  /** Assemble the multivector from elements using value to index mapping.
   ** @param count Amount of the value buckets to be added.
   ** @param range Range of the value handles to be added.
   ** @param index_of Function that returns bucket index of the value by handle.
   ** @param value_of Funtion that turns value handle into a value. */
  /** @{ */
  /** This version of the function works best when array size is much larger
   ** than typical amount of values in bucket (multivector is "tall"). */
  template<std::ranges::input_range Handles, //
           class IndexOf, class ValueOf = std::identity>
  constexpr void assemble_tall(size_t count, Handles&& handles, //
                               IndexOf index_of, ValueOf value_of = {}) {
    // Compute value ranges.
    /// First compute how many values there are per each index.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    par::for_each(handles, [&](auto handle) {
      const auto index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      __sync_fetch_and_add(&val_ranges_[index + 1], 1);
    });
    /// Perform a partial sum of the computed values to form ranges.
    for (size_t index = 2; index < val_ranges_.size(); ++index) {
      val_ranges_[index] += val_ranges_[index - 1];
    }
    // Place values according to the ranges.
    /// Place each value into position of the first element of it's index range,
    /// then increment the position.
    const auto num_vals = val_ranges_.back();
    vals_.resize(num_vals); // No need to clear the `vals_`!
    par::for_each(handles, [&](auto handle) {
      const auto index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      const auto addr = __sync_fetch_and_add(&val_ranges_[index], 1);
      vals_[addr] = value_of(handle);
    });
    /// Fix value ranges, since after incrementing the entire array is shifted
    /// left. So we need to shift it right and restore the leading zero.
    std::shift_right(val_ranges_.begin(), val_ranges_.end(), 1);
    val_ranges_[0] = 0;
  }
  /** This version of the function works best when array size is much less
   ** than typical amount of values in bucket (multivector is "wide"). */
  template<std::ranges::input_range Handles, //
           class IndexOf, class ValueOf = std::identity>
  constexpr void assemble_wide(size_t count, Handles&& handles, //
                               IndexOf index_of, ValueOf value_of = {}) {
    // Compute value ranges.
    /// First compute how many values there are per each index per each thread.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    // TODO: `std::array<size_t, 8>` should be replaced!
    std::vector<std::array<size_t, 8>> val_ranges_per_thread(count + 1);
    par::static_for_each(handles, [&](auto handle) {
      const auto index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      val_ranges_per_thread[index][par::thread_index()]++;
    });
    /// Perform a partial sum of the computed values to form ranges.
    for (size_t index = 1; index < val_ranges_.size(); ++index) {
      /// First, compute partial sums across the threads.
      auto& thread_ranges = val_ranges_per_thread[index - 1];
      for (size_t thread = 1; thread < par::num_threads(); ++thread) {
        thread_ranges[thread] += thread_ranges[thread - 1];
      }
      /// Second, form the per index ranges.
      val_ranges_[index] = thread_ranges.back();
      val_ranges_[index] += val_ranges_[index - 1];
      /// Third, adjust partial sums per threads threads to form the ranges.
      std::shift_right(thread_ranges.begin(), thread_ranges.end(), 1);
      thread_ranges[0] = 0;
      for (auto& first : thread_ranges) first += val_ranges_[index - 1];
    }
    // Place values according to the ranges.
    /// Place each value into position of the first element of it's index range,
    /// then increment the position.
    const auto num_vals = val_ranges_.back();
    vals_.resize(num_vals); // No need to clear the `vals_`!
    par::static_for_each(handles, [&](auto handle) {
      const auto index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      const auto addr = val_ranges_per_thread[index][par::thread_index()]++;
      vals_[addr] = value_of(handle);
    });
  }
  /** @} */

}; // class Multivector

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
