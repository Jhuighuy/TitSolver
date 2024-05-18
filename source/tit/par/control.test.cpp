/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <mutex>
#include <ranges>
#include <set>
#include <thread>

#include "tit/core/basic_types.hpp"

#include "tit/par/control.hpp"
#include "tit/par/thread.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::num_threads") {
  for (size_t num_threads : std::views::iota(1, 4)) {
    // Set number of threads and check that the value was actually set.
    par::set_num_threads(num_threads);
    CHECK(par::num_threads() == num_threads);
    // Run the parallel loop and record IDs of the worker threads.
    std::mutex mutex{};
    std::set<std::thread::id> worker_thread_ids{};
    par::for_each(std::views::iota(0, 10), [&](int /*unused*/) {
      // Pretend we are doing some work.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      // Record ID of the current thread.
      const std::scoped_lock lock{mutex};
      worker_thread_ids.insert(std::this_thread::get_id());
    });
    // Amount of the recorded worker threads should match the value that we've
    // specified.
    CHECK(worker_thread_ids.size() == num_threads);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
