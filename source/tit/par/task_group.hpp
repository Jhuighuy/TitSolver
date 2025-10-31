/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/task_group.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Task function.
template<class Task>
concept task =
    std::invocable<Task> && std::same_as<std::invoke_result_t<Task>, void>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Task run mode.
enum class RunMode : uint8_t {
  parallel,
  sequential,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parallel task group.
class TaskGroup final {
public:

  /// Run the task.
  template<class Task>
    requires task<Task&&>
  void run(Task&& task, RunMode mode = RunMode::parallel) {
    TIT_ASSERT(group_ != nullptr, "Task group was moved away!");
    switch (mode) {
      case RunMode::parallel:   group_->run(std::forward<Task>(task)); break;
      case RunMode::sequential: std::invoke(std::forward<Task>(task)); break;
      default:                  std::unreachable();
    }
  }

  /// Wait for the group to finish.
  void wait() {
    TIT_ASSERT(group_ != nullptr, "Task group was moved away!");
    group_->wait();
  }

private:

  std::unique_ptr<tbb::task_group> group_ =
      std::make_unique<typename decltype(group_)::element_type>();

}; // class TaskGroup

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
