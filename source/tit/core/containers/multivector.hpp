/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <initializer_list>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
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
  constexpr Multivector() = default;

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
  constexpr void clear() noexcept { // NOLINT(*-exception-escape)
    TIT_ASSERT(!val_ranges_.empty(), "Value ranges must not be empty!");
    val_ranges_.clear(), val_ranges_.push_back(0);
    vals_.clear();
  }

  /// Append a new bucket to the multivector.
  template<std::ranges::input_range Bucket>
    requires std::constructible_from<Val,
                                     std::ranges::range_reference_t<Bucket>>
  constexpr void append_bucket(Bucket&& bucket) {
    TIT_ASSUME_UNIVERSAL(Bucket, bucket);
#ifndef __clang__
    std::ranges::copy(bucket, std::back_inserter(vals_));
#else
    /// @todo For some reason, Clang cannot properly optimize the above code.
    const auto old_size = vals_.size();
    vals_.resize(old_size + bucket.size());
    std::ranges::copy(bucket, vals_.begin() + old_size);
#endif
    val_ranges_.push_back(vals_.size());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Build the multivector from a range of buckets.
  template<par::basic_range Buckets>
  void assign_buckets_par(Buckets&& buckets) {
    TIT_ASSUME_UNIVERSAL(Buckets, buckets);

    // Compute the total number of values.
    size_t num_values = 0;
    val_ranges_.clear(), val_ranges_.resize(std::size(buckets) + 1);
    for (const auto& [index, bucket] : std::views::enumerate(buckets)) {
      const auto bucket_size = std::size(bucket);
      val_ranges_[index + 1] = num_values + bucket_size;
      num_values += bucket_size;
    }

    // Copy the values.
    vals_.resize(num_values);
    par::for_each( //
        std::views::enumerate(buckets),
        [this](const auto& index_and_bucket) {
          const auto& [index, bucket] = index_and_bucket;
          std::ranges::copy(bucket,
                            std::next(vals_.begin(), val_ranges_[index]));
        });
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
      auto& position = val_ranges_[index + 1];
      vals_[position] = value;
      position += 1;
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
      auto& position = val_ranges_[index + 1];
      vals_[par::fetch_and_add(position, 1)] = value;
    });
    val_ranges_.pop_back();
  }

  /// Build the multivector from pairs of bucket indices and values.
  ///
  /// This version of the function works best when array size is much less
  /// than typical size of a bucket (multivector is "wide").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param pairs Range of the pairs of bucket indices and values.
  template<par::range Pairs>
  constexpr void assign_pairs_par_wide(size_t count, Pairs&& pairs) {
    TIT_ASSUME_UNIVERSAL(Pairs, pairs);

    // Compute how many values there are per each index per each thread.
    static Mdvector<size_t, 2> per_thread_ranges{};
    const auto num_threads = par::num_threads();
    per_thread_ranges.assign(num_threads, count + 1);
    // Note: here we cannot use `std::views::keys` because
    //       `par::static_for_each` requires either a sized random access range
    //       or a join view over a sized random access range of ranges.
    par::static_for_each(pairs, [count](size_t thread, const auto& pair) {
      const auto index = std::get<0>(pair);
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      per_thread_ranges[thread, index] += 1;
    });

    // Compute the bucket ranges from the bucket sizes.
    val_ranges_.clear(), val_ranges_.resize(count + 1);
    for (size_t offset = 0, index = 0; index < count; ++index) {
      for (size_t thread = 0; thread < num_threads; ++thread) {
        per_thread_ranges[thread, index] =
            std::exchange(offset, offset + per_thread_ranges[thread, index]);
      }
      val_ranges_[index + 1] = offset;
    }

    // Place each value into position of the first element of it's index
    // range, then increment the position.
    vals_.resize(val_ranges_.back());
    par::static_for_each(pairs, [count, this](size_t thread, const auto& pair) {
      const auto& [index, value] = pair;
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      auto& position = per_thread_ranges[thread, index];
      vals_[position] = value;
      position += 1;
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
