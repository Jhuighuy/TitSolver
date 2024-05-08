/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>

#include "tit/core/checks.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/system.hpp"

#include "tit/par/control.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_main(int argc,
              char** argv,
              main_func_t const& main_func) noexcept -> int {
  // Setup signal handler.
  FatalSignalHandler const handler{};
  // Enable profiling.
  if (std::getenv("TIT_ENABLE_PROFILER") != nullptr) { // NOLINT(*-mt-unsafe)
    Profiler::enable();
  }
  // Setup parallelism.
  par::set_num_threads(8);
  // Run the main function.
  TIT_ENSURE(main_func != nullptr, "Main function must be specified!");
  return main_func(argc, argv);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
