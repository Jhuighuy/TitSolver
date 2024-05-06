/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/utils.hpp"

#include "tit/par/atomic.hpp"
#include "tit/par/control.hpp"
#include "tit/par/thread.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed vector that can handle multiple elements at a single position.
template<class Val>
class Multivector {
private:

  std::vector<size_t> val_ranges_{0};
  std::vector<Val> vals_;

public:

  /// Multivector size.
  constexpr auto size() const noexcept -> size_t {
    return val_ranges_.size() - 1;
  }

  /// Is multivector empty?
  constexpr auto empty() const noexcept -> bool {
    return val_ranges_.size() == 1;
  }

  /// Clear the multivector.
  constexpr void clear() noexcept { // NOLINT(*-exception-escape)
    val_ranges_.clear(), val_ranges_.push_back(0);
    vals_.clear();
  }

  /// Range of values at index.
  /// @{
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
  /// @}

  /// Append values to the multivector.
  template<std::ranges::input_range Vals>
    requires std::indirectly_copyable<std::ranges::iterator_t<Vals>,
                                      std::ranges::iterator_t<std::vector<Val>>>
  constexpr void push_back(Vals&& vals) {
    TIT_ASSUME_UNIVERSAL(Vals, vals);
    std::ranges::copy(vals, std::back_inserter(vals_));
    val_ranges_.push_back(vals_.size());
  }

  /// Sort values of each index.
  template<class Cmp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<std::ranges::iterator_t<std::vector<Val>>, Cmp, Proj>
  constexpr void sort(Cmp cmp = {}, Proj proj = {}) noexcept {
    par::for_each(std::views::iota(0UZ, size()), [=, this](size_t index) {
      std::ranges::sort((*this)[index], cmp, proj);
    });
  }

  /// Assemble the multivector from elements using value to index mapping in
  /// parallel.
  ///
  /// This version of the function works best when array size is much larger
  /// than typical amount of values in bucket (multivector is "tall").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param handles Range of the value handles to be added.
  /// @param index_of Function that returns bucket index of the handle value.
  /// @param value_of Function that turns value handle into a value.
  template<par::input_range Handles,
           std::regular_invocable<std::ranges::range_value_t<Handles>> IndexOf,
           std::regular_invocable<std::ranges::range_value_t<Handles>> ValueOf =
               std::identity>
    requires std::constructible_from<
                 size_t, std::invoke_result_t<
                             IndexOf, std::ranges::range_value_t<Handles>>> &&
             std::assignable_from<
                 Val&, std::invoke_result_t<
                           ValueOf, std::ranges::range_value_t<Handles>>>
  constexpr void assemble_tall(size_t count, Handles&& handles,
                               IndexOf index_of, ValueOf value_of = {}) {
    TIT_ASSUME_UNIVERSAL(Handles, handles);
    // Compute value ranges.
    /// First compute how many values there are per each index.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    par::for_each(handles, [&](auto handle) {
      size_t const index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      par::sync_fetch_and_add(val_ranges_[index + 1], 1);
    });
    /// Perform a partial sum of the computed values to form ranges.
    for (size_t index = 2; index < val_ranges_.size(); ++index) {
      val_ranges_[index] += val_ranges_[index - 1];
    }
    // Place values according to the ranges.
    /// Place each value into position of the first element of it's index range,
    /// then increment the position.
    auto const num_vals = val_ranges_.back();
    vals_.resize(num_vals); // No need to clear the `vals_`!
    par::for_each(handles, [&](auto handle) {
      auto const index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      auto const addr = par::sync_fetch_and_add(val_ranges_[index], 1);
      vals_[addr] = value_of(handle);
    });
    /// Fix value ranges, since after incrementing the entire array is shifted
    /// left. So we need to shift it right and restore the leading zero.
    std::shift_right(val_ranges_.begin(), val_ranges_.end(), 1);
    val_ranges_[0] = 0;
  }

  /// Assemble the multivector from elements using value to index mapping in
  /// parallel.
  ///
  /// This version of the function works best when array size is much less
  /// than typical amount of values in bucket (multivector is "wide").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param handles Range of the value handles to be added.
  /// @param index_of Function that returns bucket index of the handle value.
  /// @param value_of Function that turns value handle into a value.
  template<par::input_range Handles,
           std::regular_invocable<std::ranges::range_value_t<Handles>> IndexOf,
           std::regular_invocable<std::ranges::range_value_t<Handles>> ValueOf =
               std::identity>
    requires std::constructible_from<
                 size_t, std::invoke_result_t<
                             IndexOf, std::ranges::range_value_t<Handles>>> &&
             std::assignable_from<
                 Val&, std::invoke_result_t<
                           ValueOf, std::ranges::range_value_t<Handles>>>
  constexpr void assemble_wide(size_t count, Handles&& handles,
                               IndexOf index_of, ValueOf value_of = {}) {
    TIT_ASSUME_UNIVERSAL(Handles, handles);
    // Compute value ranges.
    /// First compute how many values there are per each index per each thread.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    static Mdvector<size_t, 2> val_ranges_per_thread{};
    val_ranges_per_thread.assign(count + 1, par::num_threads());
    par::static_for_each(handles, [&](auto handle) {
      size_t const index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      val_ranges_per_thread[index, par::thread_index()]++;
    });
    /// Perform a partial sum of the computed values to form ranges.
    for (size_t index = 1; index < val_ranges_.size(); ++index) {
      /// First, compute partial sums across the threads.
      auto thread_ranges = val_ranges_per_thread[index - 1];
      for (size_t thread = 1; thread < par::num_threads(); ++thread) {
        thread_ranges[thread] += thread_ranges[thread - 1];
      }
      /// Second, form the per index ranges.
      val_ranges_[index] = val_ranges_[index - 1] + thread_ranges.back();
      /// Third, adjust partial sums per threads threads to form the ranges.
      std::shift_right(thread_ranges.begin(), thread_ranges.end(), 1);
      thread_ranges[0] = 0;
      for (auto& first : thread_ranges) first += val_ranges_[index - 1];
    }
    // Place values according to the ranges.
    /// Place each value into position of the first element of it's index range,
    /// then increment the position.
    auto const num_vals = val_ranges_.back();
    vals_.resize(num_vals); // No need to clear the `vals_`!
    par::static_for_each(handles, [&](auto handle) {
      auto const index = index_of(handle);
      TIT_ASSERT(index < count, "Index of the value is out of expected range.");
      auto const addr = val_ranges_per_thread[index, par::thread_index()]++;
      vals_[addr] = value_of(handle);
    });
  }

}; // class Multivector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
