/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once
#include "thread_omp.hpp"
#if 0

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>

#include "tit/core/config.hpp"
#include "tit/core/types.hpp"

#if TIT_ENABLE_TBB
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/partitioner.h>
#endif

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if TIT_ENABLE_TBB
// Number of threads.
size_t _num_threads = 1;
// Index of the current thread.
thread_local size_t _thread_index = SIZE_MAX;
#endif

/** Number of threads. */
constexpr auto num_threads() noexcept -> size_t {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    return 1;
#if TIT_ENABLE_TBB
  } else {
    return _num_threads;
  }
#endif
}

/** Current thread index. */
constexpr auto thread_index() noexcept -> size_t {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    return 0;
#if TIT_ENABLE_TBB
  } else {
    return _thread_index;
  }
#endif
}

/** Wrapper for the `main` that sets up parallelism. */
template<std::invocable Func>
auto main(int argc, char** argv, Func&& func) noexcept -> int {
#if TIT_ENABLE_TBB
  // TODO: correctly set maximum amount of cores.
  tbb::global_control gc{tbb::global_control::max_allowed_parallelism,
                         _num_threads};
#endif
  return func();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Invoke functions in parallel (inside the current process). */
template<std::invocable... Funcs>
constexpr void invoke(Funcs&&... funcs) noexcept {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    (funcs(), ...);
#if TIT_ENABLE_TBB
  } else {
    tbb::parallel_invoke(funcs...);
  }
#endif
}

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
constexpr void for_each(Range&& range, Func&& func,
                        [[maybe_unused]] size_t grain_size = 1) noexcept {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    std::ranges::for_each(range, func);
#if TIT_ENABLE_TBB
  } else {
    const auto blocked_range =
        tbb::blocked_range{range.begin(), range.end(), grain_size};
    tbb::parallel_for(blocked_range, [&](auto subrange) {
      std::ranges::for_each(subrange, func);
    });
  }
#endif
}
// IWYU's clang has no `std::ranges::join_view`.
#if !TIT_IWYU
template<class Range, class Func>
constexpr void for_each(std::ranges::join_view<Range> range, Func&& func,
                        size_t grain_size = 1) noexcept {
  for_each(
      std::move(range).base(),
      [&](auto subrange) { std::ranges::for_each(subrange, func); },
      grain_size);
}
#endif
/** @} */

/** Iterate through the range in parallel (static partitioning). */
/** @{ */
template<class Range, class Func>
  requires _can_par<Range, Func>
constexpr void static_for_each(Range&& range, Func&& func) noexcept {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    std::ranges::for_each(range, func);
#if TIT_ENABLE_TBB
  } else {
    const auto partition_size = range.size() / num_threads();
    const auto partition_rem = range.size() % num_threads();
    const auto partition_first = [&](size_t thread) {
      return thread * partition_size + std::min(thread, partition_rem);
    };
    static auto partitioner = tbb::static_partitioner{};
    tbb::parallel_for(
        size_t{0}, num_threads(), size_t{1},
        [&](size_t thread) {
          _thread_index = thread;
          std::for_each(range.begin() + partition_first(thread),
                        range.begin() + partition_first(thread + 1), func);
          _thread_index = SIZE_MAX;
        },
        partitioner);
  }
#endif
}
// IWYU's clang has no `std::ranges::join_view`.
#if !TIT_IWYU
template<class Range, class Func>
constexpr void static_for_each(std::ranges::join_view<Range> range,
                               Func&& func) noexcept {
  static_for_each(std::move(range).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
#endif
/** @} */

template<
    std::ranges::input_range Range,
    class /*std::indirectly_unary_invocable<std::ranges::iterator_t<Range>>*/
    Func>
constexpr void block_for_each(Range&& range, Func&& func) noexcept {
#if TIT_ENABLE_TBB
  invoke([&] { std::ranges::for_each(range[0], func); },
         [&] { std::ranges::for_each(range[1], func); },
         [&] { std::ranges::for_each(range[2], func); },
         [&] { std::ranges::for_each(range[3], func); },
         [&] { std::ranges::for_each(range[4], func); },
         [&] { std::ranges::for_each(range[5], func); },
         [&] { std::ranges::for_each(range[6], func); },
         [&] { std::ranges::for_each(range[7], func); });
  std::ranges::for_each(range[8], func);
#else
  for (auto x : range) for_each(x, func);
#endif
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
#endif
