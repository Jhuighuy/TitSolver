/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <cstring>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

#include "tit/par/control.hpp"
#include "tit/par/task_group.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::TaskGroup") {
  // Explicitly enable parallelism.
  par::set_num_threads(4);
  SUBCASE("basic") {
    // Run the parallel tasks and record IDs of the worker threads.
    std::mutex mutex{};
    std::set<std::thread::id> worker_thread_ids{};
    const auto main_thread_id = std::this_thread::get_id();
    // Create the task group and run the tasks.
    par::TaskGroup group{};
    for (int i = 0; i < 20; ++i) {
      // Define the task.
      const auto parallel = i % 10 != 0;
      auto task = [&, parallel] {
        // Pretend we are doing some work.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // Check parallelism.
        const auto this_thread_id = std::this_thread::get_id();
        if (parallel) {
          // Record ID of the current thread.
          const std::scoped_lock lock{mutex};
          worker_thread_ids.insert(this_thread_id);
        } else {
          // Ensure sequential tasks run on the main thread.
          CHECK(this_thread_id == main_thread_id);
        }
      };
      // Use different overloads of the `run` method.
      if (parallel && i % 2 == 0) group.run(std::move(task));
      else group.run(parallel, std::move(task));
    }
    // Ensure the tasks have finished.
    group.wait();
    REQUIRE(!worker_thread_ids.empty());
    // Ensure the tasks have been executed in parallel.
    CHECK(worker_thread_ids.size() > 1);
  }
  SUBCASE("exceptions") {
    try {
      par::TaskGroup group{};
      for (int i = 0; i < 20; ++i) {
        group.run([i] {
          // Pretend we are doing some work.
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          // Some of the tasks will throw an exception.
          if (i == 14) throw std::runtime_error{"Task failed!"};
        });
      }
      // Ensure the tasks have finished.
      group.wait();
      FAIL("Task should have thrown an exception!");
    } catch (const std::runtime_error& e) {
      // Ensure the exception was caught.
      CHECK(std::strcmp(e.what(), "Task failed!") == 0);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
