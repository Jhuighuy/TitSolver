/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iostream> // IWYU pragma: keep
#include <iterator>
#include <ranges>

#include "tit/core/config.hpp"
#include "tit/core/types.hpp"

// IWYU pragma: begin_keep
#if TIT_ENABLE_MPI
#include <mpi.h>
#endif
#if TIT_ENABLE_TBB
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/partitioner.h>
#endif
// IWYU pragma: end_keep

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Number of threads.
size_t _num_threads = 8;

/** Wrapper for the `main` that sets up parallelism. */
template<std::invocable Func>
auto main(int argc, char** argv, Func&& func) noexcept -> int {
#if TIT_ENABLE_MPI
  const auto result = MPI_Init(&argc, &argv);
  if (result != MPI_SUCCESS) {
    std::cerr << "MPI_Init failed: " << result << std::endl;
    return 1;
  }
#endif
#if TIT_ENABLE_TBB
  // TODO: correctly set maximum amount of cores.
  tbb::global_control gc{tbb::global_control::max_allowed_parallelism,
                         _num_threads};
#endif
  auto result = func();
#if TIT_ENABLE_MPI
  MPI_Finalize();
#endif
  return result;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Current process index. */
constexpr auto proc_index() noexcept -> size_t {
#if TIT_ENABLE_MPI
  if consteval {
#endif
    return 0;
#if TIT_ENABLE_MPI
  } else {
    static auto proc_index = [] {
      int proc_index;
      const auto result = MPI_Comm_rank(MPI_COMM_WORLD, &proc_index);
      if (result != MPI_SUCCESS) {
        std::cerr << "MPI_Comm_rank failed: " << result << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
      std::cout << "My process index is " << proc_index << "." << std::endl;
      return static_cast<size_t>(proc_index);
    }();
    return proc_index;
  }
#endif
}

/** Is current process main one? */
constexpr auto is_main_proc() noexcept -> bool {
  return proc_index() == 0;
}

/** Number of the processes. */
constexpr auto num_proc() noexcept -> size_t {
#if TIT_ENABLE_MPI
  if consteval {
#endif
    return 1;
#if TIT_ENABLE_MPI
  } else {
    static auto num_proc = [] {
      int num_proc;
      const auto result = MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
      if (result != MPI_SUCCESS) {
        std::cerr << "MPI_Comm_size failed: " << result << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
      if (is_main_proc()) {
        std::cout << "Running on " << num_proc << " processes. " << std::endl;
      }
      return static_cast<size_t>(num_proc);
    }();
    return num_proc;
  }
#endif
}

/** Wait for all subprocesses at the barrier. */
constexpr auto mp_barrier() noexcept {
#if TIT_ENABLE_MPI
  if !consteval {
    MPI_Barrier(MPI_COMM_WORLD);
  }
#endif
}

/** Execute function in order with subprocess indices. */
template<class Func>
constexpr void mp_ordered(Func&& func) noexcept {
  for (size_t i = 0; i < num_proc(); ++i) {
    if (i == proc_index()) func();
    mp_barrier();
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Index of the current thread.
thread_local size_t _thread_index = SIZE_MAX;

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
#if 1
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
