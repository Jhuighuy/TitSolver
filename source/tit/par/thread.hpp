/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts> // IWYU pragma: keep
#include <cstdint>
#include <iterator> // IWYU pragma: keep
#include <ranges>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/partitioner.h>

#include "tit/core/basic_types.hpp"

#include "tit/par/control.hpp"

namespace tit::par {

// NOLINTBEGIN(*-missing-std-forward)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Index of the current thread.
extern thread_local size_t _thread_index;

/// Current thread index.
inline auto thread_index() noexcept -> size_t {
  return _thread_index;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Range>
concept input_range = std::ranges::input_range<Range>;

template<class Range, class Func>
concept _can_par =
    std::ranges::input_range<Range> && std::ranges::sized_range<Range> &&
    std::indirectly_unary_invocable<Func, std::ranges::iterator_t<Range>>;

/** Iterate through the range in parallel (dynamic partitioning). */
/** @{ */
template<class Range, class Func>
  requires _can_par<Range, Func>
void for_each(Range&& range, Func&& func) noexcept {
  auto const blocked_range = tbb::blocked_range{range.begin(), range.end()};
  tbb::parallel_for(blocked_range, [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
template<class Range, class Func>
void for_each(std::ranges::join_view<Range> range, Func&& func) noexcept {
  for_each(std::move(range).base(),
           [&](auto subrange) { std::ranges::for_each(subrange, func); });
}
/** @} */

/** Iterate through the range in parallel (static partitioning). */
/** @{ */
template<class Range, class Func>
  requires _can_par<Range, Func>
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

template<
    std::ranges::input_range Range,
    class /*std::indirectly_unary_invocable<std::ranges::iterator_t<Range>>*/
    Func>
void block_for_each(Range&& range, Func&& func) noexcept {
  tbb::parallel_invoke( //
      [&] { std::ranges::for_each(range[0], func); },
      [&] { std::ranges::for_each(range[1], func); },
      [&] { std::ranges::for_each(range[2], func); },
      [&] { std::ranges::for_each(range[3], func); },
      [&] { std::ranges::for_each(range[4], func); },
      [&] { std::ranges::for_each(range[5], func); },
      [&] { std::ranges::for_each(range[6], func); },
      [&] { std::ranges::for_each(range[7], func); });
  std::ranges::for_each(range[8], func);
}

#if 0
template<class Range>
concept block_input_range = std::ranges::input_range<Range>;

template<block_input_range Range,
         std::indirectly_regular_unary_invocable<
             std::ranges::iterator_t<std::ranges::range_value_t<Range>>>
             Func>
 void block_for_each(Range&& range, Func&& func) noexcept {
  // Split the range in chunks according to the number of threads and
  // walk though the chunks sequentially.
  auto const chucked_range =
      std::views::chunk(std::forward<Range>(range), num_threads());
  for (auto&& chuck : chucked_range) {
    // Subranges inside each chunk are supposed to be independent thus
    // are processed in parallel.
    static_for_each(chuck, [&](auto subrange) {
      std::ranges::for_each(subrange, func); //
    });
  }
}
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-missing-std-forward)

} // namespace tit::par
