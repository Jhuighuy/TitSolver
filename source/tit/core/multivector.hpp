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
#include <span>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class Val, class Handles, class IndexOf, class ValueOf = std::identity>
concept can_assemble_multivec =
    std::is_object_v<Val> && par::range<Handles> &&
    std::regular_invocable<IndexOf, std::ranges::range_reference_t<Handles>> &&
    std::convertible_to<
        std::invoke_result_t<IndexOf, std::ranges::range_reference_t<Handles>>,
        size_t> &&
    std::regular_invocable<ValueOf, std::ranges::range_reference_t<Handles>> &&
    std::convertible_to<
        std::invoke_result_t<ValueOf, std::ranges::range_reference_t<Handles>>,
        Val>;
} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed vector that can handle multiple elements at a single position.
template<class Val>
class Multivector {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Multivector size.
  constexpr auto size() const noexcept -> size_t {
    return val_ranges_.size() - 1;
  }

  /// Range of bucket sizes.
  constexpr auto sizes() const noexcept {
    return val_ranges_ | std::views::adjacent_transform<2>(
                             [](size_t a, size_t b) { return b - a; });
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
  constexpr auto operator[](size_t index) noexcept -> std::span<Val> {
    TIT_ASSERT(index < size(), "Multivector index is out of range.");
    return std::span{vals_.begin() + val_ranges_[index],
                     vals_.begin() + val_ranges_[index + 1]};
  }
  constexpr auto operator[](size_t index) const noexcept
      -> std::span<const Val> {
    TIT_ASSERT(index < size(), "Multivector index is out of range.");
    return std::span{vals_.begin() + val_ranges_[index],
                     vals_.begin() + val_ranges_[index + 1]};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    par::for_each(std::views::iota(size_t{0}, size()),
                  [cmp, proj, this](size_t index) {
                    std::ranges::sort((*this)[index], cmp, proj);
                  });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
  template<class Handles, class IndexOf, class ValueOf = std::identity>
    requires impl::can_assemble_multivec<Val, Handles&&, IndexOf, ValueOf>
  constexpr void assemble_tall(size_t count,
                               Handles&& handles,
                               IndexOf index_of,
                               ValueOf value_of = {}) {
    TIT_ASSUME_UNIVERSAL(Handles, handles);
    // Compute value ranges.
    /// First compute how many values there are per each index.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    par::for_each(handles, [&](const auto& handle) {
      const auto index = static_cast<size_t>(index_of(handle));
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      par::fetch_and_add(val_ranges_[index + 1], 1);
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
    par::for_each(handles, [&](const auto& handle) {
      const auto index = static_cast<size_t>(index_of(handle));
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      const auto addr = par::fetch_and_add(val_ranges_[index], 1);
      vals_[addr] = static_cast<Val>(value_of(handle));
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
  template<class Handles, class IndexOf, class ValueOf = std::identity>
    requires impl::can_assemble_multivec<Val, Handles&&, IndexOf, ValueOf>
  constexpr void assemble_wide(size_t count,
                               Handles&& handles,
                               IndexOf index_of,
                               ValueOf value_of = {}) {
    TIT_ASSUME_UNIVERSAL(Handles, handles);
    // Compute value ranges.
    /// First compute how many values there are per each index per each thread.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    TIT_SAVED_VARIABLE(val_ranges_per_thread, Mdvector<size_t, 2>{});
    val_ranges_per_thread.assign(count + 1, par::num_threads());
    par::static_for_each(handles, [&](size_t thread_index, const auto& handle) {
      const auto index = static_cast<size_t>(index_of(handle));
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      val_ranges_per_thread[index, thread_index]++;
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
    const auto num_vals = val_ranges_.back();
    vals_.resize(num_vals); // No need to clear the `vals_`!
    par::static_for_each(handles, [&](size_t thread_index, const auto& handle) {
      const auto index = static_cast<size_t>(index_of(handle));
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      auto& addr = val_ranges_per_thread[index, thread_index];
      vals_[addr] = static_cast<Val>(value_of(handle));
      addr += 1;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::vector<size_t> val_ranges_{0};
  std::vector<Val> vals_;

}; // class Multivector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
