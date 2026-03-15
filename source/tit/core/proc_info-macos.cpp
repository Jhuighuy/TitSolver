/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __APPLE__

#include <libproc.h>
#include <mach/kern_return.h>
#include <mach/mach_time.h>
#include <sys/proc_info.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/posix.hpp"
#include "tit/core/proc_info.hpp"

namespace tit::proc_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto to_ns(uint64_t absolute_time) -> uint64_t {
  mach_timebase_info_data_t timebase{};
  const auto status = mach_timebase_info(&timebase);
  TIT_ENSURE(status == KERN_SUCCESS, "Unable to query Mach timebase info.");
  return absolute_time * timebase.numer / timebase.denom; // codespell:ignore
}

} // namespace

auto query_usage(pid_t pid) -> UsageSnapshot {
  TIT_ASSERT(pid > 0, "Invalid process ID!");

  proc_taskinfo task_info{};
  const auto result =
      proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info));
  TIT_ENSURE(result == sizeof(task_info),
             "Unable to query process usage for PID {}.",
             pid);

  return {
      .cpu_time_ns =
          to_ns(task_info.pti_total_user + task_info.pti_total_system),
      .memory_bytes = task_info.pti_resident_size,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::proc_info

#endif // ifdef __APPLE__
