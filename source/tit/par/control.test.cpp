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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("par::num_threads") {
  for (size_t num_threads : std::views::iota(1, 5)) {
    // Set number of threads and check that the value was actually set.
    par::set_num_threads(num_threads);
    CHECK(par::num_threads() == num_threads);
    // Run the parallel loop and count how many worker threads are actually
    // used.
    std::mutex mutex{};
    std::set<std::thread::id> worker_thread_ids{};
    par::for_each(std::views::iota(0, 10), [&](int /*unused*/) {
      // Simulate some work.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Save ID of the current thread.
      std::scoped_lock const lock{mutex};
      worker_thread_ids.insert(std::this_thread::get_id());
    });
    // At the end, we should have the same amount of the unique thread IDs
    // as the worker threads.
    CHECK(worker_thread_ids.size() == num_threads);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
