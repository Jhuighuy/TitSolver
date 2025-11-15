/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <optional>

#include <oneapi/tbb/global_control.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/sys_info.hpp"
#include "tit/par/control.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void init() {
  par::set_num_threads(get_env("TIT_NUM_THREADS", sys_info::cpu_perf_cores()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto num_threads() noexcept -> size_t {
  return tbb::global_control::active_value(
      tbb::global_control::max_allowed_parallelism);
}

void set_num_threads(size_t value) {
  TIT_ASSERT(value > 0, "Invalid number of the worker threads!");
  if (num_threads() == value) return;
  static std::optional<tbb::global_control> control{};
  control.emplace(tbb::global_control::max_allowed_parallelism, value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
