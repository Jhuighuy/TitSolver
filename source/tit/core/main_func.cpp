/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/signal.hpp"
#include "tit/core/stats.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_main(int argc, char** argv, const MainFunc& main_func) -> int {
  // Setup signal handler.
  const FatalSignalHandler handler{};

  // Setup terminate handler.
  const TerminateHandler terminate_handler{};

  // Enable statistics.
  if (get_env_bool("TIT_ENABLE_STATS", false)) Stats::enable();

  // Enable profiling.
  if (get_env_bool("TIT_ENABLE_PROFILER", false)) Profiler::enable();

  // Setup parallelism.
  par::set_num_threads(get_env_uint("TIT_NUM_THREADS", 8));

  // Run the main function.
  TIT_ENSURE(main_func != nullptr, "Main function must be specified!");
  TIT_ENSURE(argc > 0, "Invalid number of command line arguments!");
  TIT_ENSURE(argv != nullptr, "Invalid command line arguments!");
  return main_func({const_cast<const char**>(argv), static_cast<size_t>(argc)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
