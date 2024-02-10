/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include <oneapi/tbb/task_group.h>

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Parallel task group.
\******************************************************************************/
class TaskGroup {
public:

  /** Construct a task group. */
  TaskGroup() = default;

  /** Task group is not move-constructible. */
  TaskGroup(TaskGroup&&) = delete;
  /** Task group is not move-assignable. */
  auto operator=(TaskGroup&&) -> TaskGroup& = delete;

  /** Task group is not copy-constructible. */
  TaskGroup(TaskGroup const&) = delete;
  /** Task group is not copyable. */
  auto operator=(TaskGroup const&) -> TaskGroup& = delete;

  /** Wait for the tasks and destroy the task group. */
  ~TaskGroup() {
    group_.wait();
  }

  /** Run the task. */
  /** @{ */
  template<class Func>
    requires std::invocable<Func&&> &&
             std::same_as<std::invoke_result_t<Func&&>, void>
  void run(Func&& func) {
    group_.run(std::forward<Func>(func));
  }
  template<class Func>
    requires std::invocable<Func&&> &&
             std::same_as<std::invoke_result_t<Func&&>, void>
  void run(bool cond, Func&& func) {
    if (cond) group_.run(std::forward<Func>(func));
    else std::forward<Func>(func)();
  }
  /** @} */

private:

  tbb::task_group group_;

}; // class TaskGroup

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
