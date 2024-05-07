/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstdint>
#include <ranges>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/partitioner.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

#include "tit/par/control.hpp"
#include "tit/par/task_group.hpp"

namespace tit::par {

// NOLINTBEGIN

using std::ranges::input_range;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Index of the current thread.
thread_local size_t _thread_index = SIZE_MAX;

/// Current thread index.
inline auto thread_index() noexcept -> size_t {
  return _thread_index;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the range in parallel (dynamic partitioning).
/// @{
template<class Range, class Func>
void for_each(Range&& range, Func&& func,
              [[maybe_unused]] size_t grain_size = 1) noexcept {
  auto const blocked_range =
      tbb::blocked_range{range.begin(), range.end(), grain_size};
  tbb::parallel_for(blocked_range, [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
template<class Range, class Func>
void for_each(std::ranges::join_view<Range> range, Func&& func,
              size_t grain_size = 1) noexcept {
  for_each(
      std::move(range).base(),
      [&](auto subrange) { std::ranges::for_each(subrange, func); },
      grain_size);
}
/// @}

/// Iterate through the range in parallel (static partitioning).
/// @{
template<class Range, class Func>
void static_for_each(Range&& range, Func&& func) noexcept {
  auto const partition_size = range.size() / num_threads();
  auto const partition_rem = range.size() % num_threads();
  auto const partition_first = [&](size_t thread) {
    return thread * partition_size + std::min(thread, partition_rem);
  };
  static auto partitioner = tbb::static_partitioner{};
  tbb::parallel_for(
      0UZ, num_threads(), 1UZ,
      [&](size_t thread) {
        _thread_index = thread;
        std::for_each(range.begin() + partition_first(thread),
                      range.begin() + partition_first(thread + 1), func);
        _thread_index = SIZE_MAX;
      },
      partitioner);
}
template<class Range, class Func>
void static_for_each(std::ranges::join_view<Range> range,
                     Func&& func) noexcept {
  static_for_each(std::move(range).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
/// @}

template<std::ranges::input_range Range, class Func>
void block_for_each(Range&& range, Func&& func) noexcept {
  TIT_ENSURE(num_threads() == 8, "");
  invoke([&] { std::ranges::for_each(range[0], func); },
         [&] { std::ranges::for_each(range[1], func); },
         [&] { std::ranges::for_each(range[2], func); },
         [&] { std::ranges::for_each(range[3], func); },
         [&] { std::ranges::for_each(range[4], func); },
         [&] { std::ranges::for_each(range[5], func); },
         [&] { std::ranges::for_each(range[6], func); },
         [&] { std::ranges::for_each(range[7], func); });
  std::ranges::for_each(range[8], func);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND

} // namespace tit::par
