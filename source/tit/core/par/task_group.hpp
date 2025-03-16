/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

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

} // namespace tit::par
