/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <thread>

#include "tit/par/control.hpp"
#include "tit/par/task_group.hpp"
#include "tit/testing/test.hpp"
#include "tit/testing/utils.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// Intel TBB library, there is no need to test it in detail. The only thing we
// need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::TaskGroup") {
  par::set_num_threads(4);
  SUBCASE("basic") {
    par::set_num_threads(2);

    // Ensure the tasks are executed.
    par::TaskGroup group{};

    std::thread::id task_1_thread_id{};
    group.run(SleepFunc{SleepFunc{[&task_1_thread_id] {
      task_1_thread_id = std::this_thread::get_id();
    }}});

    std::thread::id task_2_thread_id{};
    group.run(SleepFunc{[&task_2_thread_id] {
                task_2_thread_id = std::this_thread::get_id();
              }},
              par::RunMode::sequential);

    std::thread::id task_3_thread_id{};
    group.run(SleepFunc{[&task_3_thread_id] {
                task_3_thread_id = std::this_thread::get_id();
              }},
              par::RunMode::parallel);

    group.wait();

    const auto main_thread_id = std::this_thread::get_id();
    CHECK(task_2_thread_id == main_thread_id);
    CHECK((task_1_thread_id != main_thread_id ||
           task_3_thread_id != main_thread_id));
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from worker threads are caught.
    CHECK_THROWS_WITH_AS(
        [] {
          par::TaskGroup group{};
          group.run([] { throw std::runtime_error{"Task failed!"}; });
          group.wait();
          FAIL("Task should have thrown an exception!");
        }(),
        "Task failed!",
        std::runtime_error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
