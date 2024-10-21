/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed vector that can handle multiple elements at a single position.
template<class Val>
class Multivector {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an empty multivector.
  constexpr Multivector() noexcept = default;

  /// Construct a multivector from initial values.
  constexpr explicit Multivector(
      std::initializer_list<std::initializer_list<int>> buckets) {
    for (const auto& bucket : buckets) append_bucket(bucket);
  }

  /// Multivector size.
  constexpr auto size() const noexcept -> size_t {
    return val_ranges_.size() - 1;
  }

  /// Is multivector empty?
  constexpr auto empty() const noexcept -> bool {
    return val_ranges_.size() == 1;
  }

  /// Range of bucket sizes.
  constexpr auto bucket_sizes() const noexcept {
    return val_ranges_ | std::views::adjacent_transform<2>(
                             [](size_t a, size_t b) { return b - a; });
  }

  /// Buckets of values.
  constexpr auto buckets(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Bucket of values at index.
  /// @{
  constexpr auto operator[](size_t index) noexcept -> std::span<Val> {
    TIT_ASSERT(index < size(), "Bucket index is out of range!");
    return {vals_.begin() + val_ranges_[index],
            vals_.begin() + val_ranges_[index + 1]};
  }
  constexpr auto operator[](size_t index) const noexcept
      -> std::span<const Val> {
    TIT_ASSERT(index < size(), "Bucket index is out of range!");
    return {vals_.begin() + val_ranges_[index],
            vals_.begin() + val_ranges_[index + 1]};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Clear the multivector.
  constexpr void clear() noexcept {
    TIT_ASSERT(!val_ranges_.empty(), "Value ranges must not be empty!");
    val_ranges_[0] = 0;
    val_ranges_.resize(1);
    vals_.clear();
  }

  /// Append a new bucket to the multivector.
  template<std::ranges::input_range Bucket>
    requires std::constructible_from<Val,
                                     std::ranges::range_reference_t<Bucket>>
  constexpr void append_bucket(Bucket&& bucket) {
    TIT_ASSUME_UNIVERSAL(Bucket, bucket);
    if constexpr (std::ranges::sized_range<Bucket>) {
      vals_.reserve(std::max(vals_.capacity(), vals_.size() + bucket.size()));
    }
    std::ranges::copy(bucket, std::back_inserter(vals_));
    val_ranges_.push_back(vals_.size());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Build the multivector from pairs of bucket indices and values.
  ///
  /// This version runs sequentially.
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param pairs Range of the pairs of bucket indices and values.
  template<std::ranges::input_range Pairs>
  constexpr void assign_pairs_seq(size_t count, Pairs&& pairs) {
    TIT_ASSUME_UNIVERSAL(Pairs, pairs);

    // Compute how many values there are per each index.
    // Note: counts are shifted by two in order to avoid shifting the entire
    // array after assigning the values.
    val_ranges_.clear(), val_ranges_.resize(count + 2);
    for (const auto index : pairs | std::views::keys) {
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      val_ranges_[index + 2] += 1;
    }

    // Compute the bucket ranges from the bucket sizes.
    std::partial_sum(val_ranges_.begin() + 2,
                     val_ranges_.end(),
                     val_ranges_.begin() + 2);

    // Place each value into position of the first element of it's index
    // range, then increment the position.
    vals_.resize(val_ranges_.back());
    for (const auto& [index, value] : pairs) {
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      const auto position = val_ranges_[index + 1];
      val_ranges_[index + 1] += 1;
      vals_[position] = value;
    }
    val_ranges_.pop_back();
  }

  /// Build the multivector from pairs of bucket indices and values.
  ///
  /// This version of the function works best when array size is much larger
  /// than typical size in bucket (multivector is "tall").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param pairs Range of the pairs of bucket indices and values.
  template<par::basic_range Pairs>
  constexpr void assign_pairs_par_tall(size_t count, Pairs&& pairs) {
    TIT_ASSUME_UNIVERSAL(Pairs, pairs);

    // Compute how many values there are per each index.
    // Note: counts are shifted by two in order to avoid shifting the entire
    // array after assigning the values.
    val_ranges_.clear(), val_ranges_.resize(count + 2);
    par::for_each(pairs | std::views::keys, [count, this](size_t index) {
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      par::fetch_and_add(val_ranges_[index + 2], 1);
    });

    // Compute the bucket ranges from the bucket sizes.
    std::partial_sum(val_ranges_.begin() + 2,
                     val_ranges_.end(),
                     val_ranges_.begin() + 2);

    // Place each value into position of the first element of it's index
    // range, then increment the position.
    vals_.resize(val_ranges_.back());
    par::for_each(pairs, [count, this](const auto& pair) {
      const auto& [index, value] = pair;
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      const auto position = par::fetch_and_add(val_ranges_[index + 1], 1);
      vals_[position] = value;
    });
    val_ranges_.pop_back();
  }

  /// Build the multivector from pairs of bucket indices and values.
  ///
  /// This version of the function works best when array size is much less
  /// than typical size of a bucket bucket (multivector is "wide").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param pairs Range of the pairs of bucket indices and values.
  template<par::range Pairs>
  constexpr void assign_pairs_par_wide(size_t count, Pairs&& pairs) {
    TIT_ASSUME_UNIVERSAL(Pairs, pairs);

    // Compute how many values there are per each index per each thread.
    /// @todo Offset by two!
    static Mdvector<size_t, 2> val_ranges_per_thread{};
    val_ranges_per_thread.assign(count + 1, par::num_threads());
    par::static_for_each( //
        pairs | std::views::keys,
        [count](size_t thread_index, size_t index) {
          TIT_ASSERT(index < count,
                     "Index of the value is out of expected range!");
          val_ranges_per_thread[index, thread_index] += 1;
        });

    // Compute the bucket ranges from the bucket sizes.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    for (size_t index = 1; index < val_ranges_.size(); ++index) {
      // First, compute partial sums across the threads.
      auto thread_ranges = val_ranges_per_thread[index - 1];
      std::partial_sum(thread_ranges.begin(),
                       thread_ranges.end(),
                       thread_ranges.begin());

      // Second, form the per index ranges.
      val_ranges_[index] = val_ranges_[index - 1] + thread_ranges.back();

      // Third, adjust partial sums per threads threads to form the ranges.
      std::shift_right(thread_ranges.begin(), thread_ranges.end(), 1);
      thread_ranges[0] = 0;
      for (auto& first : thread_ranges) first += val_ranges_[index - 1];
    }

    // Place each value into position of the first element of it's index
    // range, then increment the position.
    vals_.resize(val_ranges_.back());
    par::static_for_each(
        pairs,
        [count, this](size_t thread_index, const auto& pair) {
          const auto& [index, value] = pair;
          TIT_ASSERT(index < count,
                     "Index of the value is out of expected range!");
          const auto position = val_ranges_per_thread[index, thread_index];
          val_ranges_per_thread[index, thread_index] += 1;
          vals_[position] = value;
        });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
  constexpr void assemble_wide(size_t count,
                               Handles&& handles,
                               IndexOf index_of,
                               ValueOf value_of = {}) {
    TIT_ASSUME_UNIVERSAL(Handles, handles);
    // Compute value ranges.
    /// First compute how many values there are per each index per each
    /// thread.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    static Mdvector<size_t, 2> val_ranges_per_thread{};
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
    /// Place each value into position of the first element of it's index
    /// range, then increment the position.
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

template<class Val>
Multivector(std::initializer_list<std::initializer_list<Val>>)
    -> Multivector<Val>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
