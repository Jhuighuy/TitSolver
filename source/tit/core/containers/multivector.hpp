/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/range.hpp"
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
      std::initializer_list<std::initializer_list<Val>> buckets) {
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
    return val_ranges_ | std::views::pairwise_transform(
                             [](size_t a, size_t b) { return b - a; });
  }

  /// Buckets of values.
  constexpr auto buckets(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Bucket of values at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Bucket index is out of range!");
    return std::span{self.vals_.begin() + self.val_ranges_[index],
                     self.vals_.begin() + self.val_ranges_[index + 1]};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Clear the multivector.
  constexpr void clear() noexcept { // NOLINT(*-exception-escape)
    TIT_ASSERT(!val_ranges_.empty(), "Value ranges must not be empty!");
    val_ranges_.clear(), val_ranges_.push_back(0);
    vals_.clear();
  }

  /// Append a new bucket to the multivector.
  template<value_range<Val> Bucket>
  constexpr void append_bucket(Bucket&& bucket) {
    TIT_ASSUME_UNIVERSAL(Bucket, bucket);
    std::ranges::copy(bucket, std::back_inserter(vals_));
    val_ranges_.push_back(vals_.size());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Build the multivector from a range of buckets.
  template<value_multirange<Val> Buckets>
    requires par::range<Buckets>
  void assign_buckets_par(Buckets&& buckets) {
    TIT_ASSUME_UNIVERSAL(Buckets, buckets);

    // Compute the total number of values.
    size_t num_values = 0;
    val_ranges_.clear(), val_ranges_.resize(std::size(buckets) + 1);
    for (const auto& [index, bucket] :
         std::views::enumerate(buckets) | std::views::as_const) {
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
  /// This version of the function works best when array size is much less
  /// than typical size of a bucket (multivector is "wide").
  ///
  /// @param count Amount of the value buckets to be added.
  /// @param pairs Range of the pairs of bucket indices and values.
  /// @{
  template<tuple_range<size_t, Val> Pairs>
    requires par::range<Pairs>
  constexpr void assign_pairs_par_wide(size_t count, Pairs&& pairs) {
    assign_pairs_wide_impl_(
        count,
        std::bind_front(par::static_for_each,
                        std::views::all(std::forward<Pairs>(pairs))));
  }
  template<tuple_multirange<size_t, Val> Range>
    requires par::range<Range>
  constexpr void assign_pairs_par_wide(size_t count,
                                       std::ranges::join_view<Range> pairs) {
    assign_pairs_wide_impl_(count, [base = std::move(pairs).base()](auto func) {
      par::static_for_each(base, [&func](size_t thread, const auto& range) {
        std::ranges::for_each(range, std::bind_front(std::ref(func), thread));
      });
    });
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  template<class ForEachPair>
  constexpr void assign_pairs_wide_impl_(size_t count,
                                         ForEachPair for_each_pair) {
    // Compute how many values there are per each index per each thread.
    static Mdvector<size_t, 2> per_thread_ranges{};
    const auto num_threads = par::num_threads();
    per_thread_ranges.assign(num_threads, count + 1);
    for_each_pair([count](size_t thread, const auto& pair) {
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
    for_each_pair([count, this](size_t thread, const auto& pair) {
      const auto& [index, value] = pair;
      TIT_ASSERT(index < count, "Index of the value is out of expected range!");
      auto& position = per_thread_ranges[thread, index];
      vals_[position] = value;
      position += 1;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  std::vector<size_t> val_ranges_{0};
  std::vector<Val> vals_;

}; // class Multivector

template<class Val>
Multivector(std::initializer_list<std::initializer_list<Val>>)
    -> Multivector<Val>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Multivector with a known upper bound on the bucket size.
template<class Val, size_t MaxBucketSize>
class CapMultivector {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a multivector.
  constexpr CapMultivector() noexcept = default;

  /// Construct a multivector with the number of buckets.
  constexpr explicit CapMultivector(size_t count) {
    assign(count);
  }

  /// Construct a multivector from initial values.
  constexpr explicit CapMultivector(
      std::initializer_list<std::initializer_list<Val>> buckets) {
    assign(buckets.size());
    for (const auto& [index, bucket] : std::views::enumerate(buckets)) {
      set_bucket(index, bucket);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Multivector size.
  constexpr auto size() const noexcept -> size_t {
    return bucket_sizes_.size();
  }

  /// Is multivector empty?
  constexpr auto empty() const noexcept -> bool {
    return bucket_sizes_.empty();
  }

  /// Range of bucket sizes.
  constexpr auto bucket_sizes() const noexcept -> std::span<const size_t> {
    return bucket_sizes_;
  }

  /// Buckets of values.
  constexpr auto buckets(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Bucket of values at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Bucket index is out of range!");
    return std::span{self.buckets_[index]} //
        .subspan(0, self.bucket_sizes_[index]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Clear the multivector.
  constexpr void clear() noexcept {
    bucket_sizes_.clear();
    buckets_.clear();
  }

  /// Assign the number of buckets.
  constexpr void assign(size_t count) {
    bucket_sizes_.clear(), bucket_sizes_.resize(count);
    buckets_.assign(count, MaxBucketSize);
  }

  /// Assign the bucket at index.
  template<value_range<Val> Bucket>
  constexpr void set_bucket(size_t index, Bucket&& bucket) {
    TIT_ASSUME_UNIVERSAL(Bucket, bucket);
    if constexpr (std::ranges::sized_range<Bucket>) {
      TIT_ASSERT(std::size(bucket) <= MaxBucketSize,
                 "Bucket size exceeds the maximum bucket size!");
    }
    std::ranges::copy(bucket, std::begin(buckets_[index]));
    bucket_sizes_[index] = std::size(bucket);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::vector<size_t> bucket_sizes_;
  Mdvector<Val, 2> buckets_;

}; // class CapMultivector

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
