/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <memory>

#include <oneapi/tbb/global_control.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

#include "tit/par/control.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto num_threads() noexcept -> size_t {
  return tbb::global_control::active_value(
      tbb::global_control::max_allowed_parallelism);
}

void set_num_threads(size_t new_num_threads) {
  TIT_ASSERT(new_num_threads > 0, "Invalid number of the worker threads!");
  if (num_threads() == new_num_threads) return;
  static std::unique_ptr<tbb::global_control> control{};
  control = std::make_unique<tbb::global_control>(
      tbb::global_control::max_allowed_parallelism, new_num_threads);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
