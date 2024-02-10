/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <gsl/pointers>

#include <oneapi/tbb/global_control.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

#include "tit/par/control.hpp"

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Since we are trying to mimic OpenMP's API, we have to use TBB's global
// control in a way it is not intended to be used. It arises a problem that if a
// destructor of a `tbb::global_control` is called during the global process
// shutdown, we end up with a segfault, since TBB is partially destroyed at this
// stage. In order to prevent the destructor call, a raw pointer is being used.

namespace {
gsl::owner<tbb::global_control*> control_ = nullptr;
} // namespace

auto num_threads() noexcept -> size_t {
  return tbb::global_control::active_value(
      tbb::global_control::max_allowed_parallelism);
}

void set_num_threads(size_t new_num_threads) {
  TIT_ASSERT(new_num_threads > 0, "Invalid number of the worker threads!");
  if (num_threads() == new_num_threads) return;
  delete control_;
  control_ = new tbb::global_control(
      tbb::global_control::max_allowed_parallelism, new_num_threads);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
