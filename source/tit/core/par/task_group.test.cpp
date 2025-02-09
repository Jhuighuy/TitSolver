/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <thread>

#include "tit/core/par/control.hpp"
#include "tit/core/par/task_group.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// Intel TBB library, there is no need to test it in detail. The only thing we
// need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::TaskGroup") {
  par::set_num_threads(4);
  SUBCASE("basic") {
    // Ensure the tasks are executed.
    par::TaskGroup group{};
    bool task_1_ran = false;
    bool task_2_ran = false;
    bool task_3_ran = false;
    group.run([&task_1_ran] { task_1_ran = true; });
    group.run(/*parallel=*/true, [&task_2_ran] { task_2_ran = true; });
    const auto main_thread_id = std::this_thread::get_id();
    group.run(/*parallel=*/false, [&task_3_ran, main_thread_id] {
      CHECK(std::this_thread::get_id() == main_thread_id);
      task_3_ran = true;
    });
    group.wait();
    CHECK(task_1_ran);
    CHECK(task_2_ran);
    CHECK(task_3_ran);
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
