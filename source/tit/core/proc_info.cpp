/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <chrono>

#include "tit/core/basic_types.hpp"
#include "tit/core/proc_info.hpp"

namespace tit::proc_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto compute_cpu_percent(const UsageSnapshot& prev,
                         const UsageSnapshot& next,
                         std::chrono::nanoseconds wall_delta) -> float64_t {
  if (wall_delta <= std::chrono::nanoseconds::zero()) return 0.0;

  const auto cpu_time_delta = next.cpu_time_ns > prev.cpu_time_ns ?
                                  next.cpu_time_ns - prev.cpu_time_ns :
                                  0;
  return std::max(0.0,
                  100.0 * static_cast<float64_t>(cpu_time_delta) /
                      static_cast<float64_t>(wall_delta.count()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::proc_info
