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
#include <iostream> // IWYU pragma: keep
#include <iterator>
#include <ranges>

#include "tit/core/config.hpp"
#include "tit/core/types.hpp"

#if TIT_ENABLE_MPI
#include <mpi.h>
#endif
#if TIT_ENABLE_TBB
#include <oneapi/tbb/blocked_range.h> // IWYU pragma: keep
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h> // IWYU pragma: keep
#include <oneapi/tbb/parallel_invoke.h>
#endif

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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
  tbb::global_control gc{tbb::global_control::max_allowed_parallelism, 8};
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

/** Iterate through the range in parallel. */
template<std::ranges::input_range Range,
         std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Func>
constexpr void for_each(Range&& range, Func&& func) noexcept {
#if TIT_ENABLE_TBB
  if consteval {
#endif
    std::ranges::for_each(range, func);
#if TIT_ENABLE_TBB
  } else {
    auto blocked_range = tbb::blocked_range{range.begin(), range.end()};
    tbb::parallel_for(blocked_range, [&](auto subrange) {
      std::ranges::for_each(subrange, func);
    });
  }
#endif
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
