/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/partitioner.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

#include "tit/par/control.hpp"

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
    TIT_ASSUME_UNIVERSAL(Range, range);
#if !(defined(__clang__) && defined(__GLIBCXX__))
    tbb::parallel_for(tbb::blocked_range{std::begin(range), std::end(range)},
                      std::bind_back(std::ranges::for_each, std::move(func)));
#else // `std::bind_back` is broken in clang with libstdc++.
    tbb::parallel_for(tbb::blocked_range{std::begin(range), std::end(range)},
                      [func = std::move(func)]<class Block>(Block&& block) {
                        TIT_ASSUME_UNIVERSAL(Block, block);
                        std::ranges::for_each(block, std::ref(func));
                      });
#endif
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
        /*last =*/thread_count,
        /*step =*/1,
        [block_first = std::move(block_first),
         func = std::move(func)](size_t thread_index) {
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
    StaticForEach{}(
        std::move(join_view).base(),
        [func = std::move(func)](size_t thread_index, const auto& range) {
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
      tbb::parallel_for<size_t>(
          /*first=*/0,
          /*last =*/std::size(chunk),
          /*step =*/1,
          [chunk = std::move(chunk), &func](size_t thread_index) {
            std::ranges::for_each(chunk[thread_index], func);
          },
          tbb::static_partitioner{});
    }
  }
};

/// @copydoc BlockForEach
inline constexpr BlockForEach block_for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
