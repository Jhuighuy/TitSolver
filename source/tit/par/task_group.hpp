/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/task_group.h>

#include "tit/core/checks.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Task function.
template<class Task>
concept task =
    std::invocable<Task> && std::same_as<std::invoke_result_t<Task>, void>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parallel task group.
class TaskGroup final {
public:

  /// Run the task.
  /// @{
  template<class Task>
    requires task<Task&&>
  void run(Task&& task) {
    TIT_ASSERT(group_ != nullptr, "Task group was moved away!");
    group_->run(std::forward<Task>(task));
  }
  template<class Task>
    requires task<Task&&>
  void run(bool parallel, Task&& task) {
    TIT_ASSERT(group_ != nullptr, "Task group was moved away!");
    if (parallel) group_->run(std::forward<Task>(task));
    else std::invoke(std::forward<Task>(task));
  }
  /// @}

  /// Wait for the group to finish.
  void wait() {
    TIT_ASSERT(group_ != nullptr, "Task group was moved away!");
    this->group_->wait();
  }

private:

  std::unique_ptr<tbb::task_group> group_ =
      std::make_unique<typename decltype(group_)::element_type>();

}; // class TaskGroup

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Invoke functions in parallel.
template<class... Tasks>
  requires (task<Tasks &&> && ...)
constexpr void invoke(Tasks&&... tasks) noexcept {
  tbb::parallel_invoke(std::forward<Tasks>(tasks)...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
