/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/par.hpp"
#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/partitioner.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/par/atomic.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Underlying range type.
template<std::ranges::view View>
using view_base_t = decltype(std::declval<View>().base());

/// Parallelizable range that could be processed directly.
template<class Range>
concept basic_range =
    std::ranges::sized_range<Range> && std::ranges::random_access_range<Range>;

/// Parallelizable range.
template<class Range>
concept range =
    basic_range<Range> ||
    // Not sure if this will be needed in the future, but it's here for now.
    (specialization_of<std::remove_cvref_t<Range>, std::ranges::join_view> &&
     basic_range<view_base_t<std::remove_cvref_t<Range>>>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the range in parallel (dynamic partitioning).
struct ForEach {
  template<basic_range Range,
           std::regular_invocable<std::ranges::range_reference_t<Range&&>> Func>
  static void operator()(Range&& range, Func func) {
    /// @todo Replace with `tbb::parallel_for_each` when it supports ranges.
    TIT_ASSUME_UNIVERSAL(Range, range);
    tbb::parallel_for(tbb::blocked_range{std::begin(range), std::end(range)},
                      std::bind_back(std::ranges::for_each, std::move(func)));
  }
};

/// @copydoc ForEach
inline constexpr ForEach for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the range in parallel (static partitioning).
struct StaticForEach {
  template<basic_range Range,
           std::invocable<size_t, std::ranges::range_reference_t<Range&&>> Func>
  static void operator()(Range&& range, Func func) {
    TIT_ASSUME_UNIVERSAL(Range, range);
    const auto thread_count = num_threads();
    auto block_first = [quotient = std::size(range) / thread_count,
                        remainder = std::size(range) % thread_count,
                        first = std::begin(range)](size_t index) {
      const auto offset = index * quotient + std::min(index, remainder);
      return first + offset;
    };
    tbb::parallel_for<size_t>(
        /*first=*/0,
        /*last=*/thread_count,
        /*step=*/1,
        [&block_first, &func](size_t thread_index) {
          std::for_each(block_first(thread_index),
                        block_first(thread_index + 1),
                        std::bind_front(std::ref(func), thread_index));
        },
        tbb::static_partitioner{});
  }

  // Not sure if this will be needed in the future, let's keep it here for now.
  template<basic_range Range,
           std::invocable<size_t,
                          std::ranges::range_reference_t<
                              std::ranges::join_view<Range>>> Func>
  static void operator()(std::ranges::join_view<Range> join_view, Func func) {
    StaticForEach{}( //
        std::move(join_view).base(),
        [&func](size_t thread_index, const auto& range) {
          std::ranges::for_each(range,
                                std::bind_front(std::ref(func), thread_index));
        });
  }
};

/// @copydoc StaticForEach
inline constexpr StaticForEach static_for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the block of ranges in parallel.
struct BlockForEach {
  template<range Range,
           std::invocable<std::ranges::range_reference_t<
               std::ranges::range_value_t<Range>>> Func>
  static void operator()(Range&& range, Func func) {
    TIT_ASSUME_UNIVERSAL(Range, range);
    for (auto chunk : std::views::chunk(range, num_threads())) {
      for_each(std::move(chunk),
               std::bind_back(std::ranges::for_each, std::cref(func)));
    }
  }
};

/// @copydoc BlockForEach
inline constexpr BlockForEach block_for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parallel copy-if.
/// Relative order of the elements in the output range is not preserved.
struct CopyIf {
  template<basic_range Range,
           std::random_access_iterator OutIter,
           class Proj = std::identity,
           std::indirect_unary_predicate<
               std::projected<std::ranges::iterator_t<Range>, Proj>> Pred>
    requires std::indirectly_copyable<std::ranges::iterator_t<Range>, OutIter>
  static auto operator()(Range&& range, OutIter out, Pred pred, Proj proj = {})
      -> OutIter {
    TIT_ASSUME_UNIVERSAL(Range, range);
    ssize_t out_index = 0;
    for_each(range, [&out_index, &out, &pred, &proj](const auto& item) {
      if (std::invoke(pred, std::invoke(proj, item))) {
        *std::next(out, fetch_and_add(out_index, 1)) = item;
      }
    });
    return std::next(out, out_index);
  }
};

/// @copydoc CopyIf
inline constexpr CopyIf copy_if{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parallel transform.
struct Transform final {
  template<basic_range Range,
           std::random_access_iterator OutIter,
           class Func,
           class Proj = std::identity>
    requires std::indirectly_writable<
                 OutIter,
                 std::indirect_result_t<
                     Func&,
                     std::projected<std::ranges::iterator_t<Range>, Proj>>>
  static auto operator()(Range&& range, OutIter out, Func func, Proj proj = {})
      -> OutIter {
    TIT_ASSUME_UNIVERSAL(Range, range);
    const auto out_end = std::next(out, std::size(range));
    for_each(std::views::zip(range, std::ranges::subrange{out, out_end}),
             [&func, &proj]<class Pair>(Pair&& source_and_dest) {
               TIT_ASSUME_UNIVERSAL(Pair, source_and_dest);
               auto&& [source, dest] = source_and_dest;
               dest = std::invoke(func, std::invoke(proj, source));
             });
    return out_end;
  }
};

/// @copydoc Transform
inline constexpr Transform transform{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
