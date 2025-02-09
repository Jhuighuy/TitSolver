/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/sys/signal.hpp"
#include "tit/core/sys/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_main(int argc, char** argv, MainFunc main_func) -> int {
  // Setup error handlers.
  const TerminateHandler terminate_handler{};
  const FatalSignalHandler signal_handler{};

  // Enable statistics.
  if (get_env("TIT_ENABLE_STATS", false)) Stats::enable();

  // Enable profiling.
  if (get_env("TIT_ENABLE_PROFILER", false)) Profiler::enable();

  // Setup parallelism.
  par::set_num_threads(get_env("TIT_NUM_THREADS", 8UZ));

  // Run the main function.
  TIT_ASSERT(main_func != nullptr, "Main function must be specified!");
  TIT_ASSERT(argc > 0, "Invalid number of command line arguments!");
  TIT_ASSERT(argv != nullptr, "Invalid command line arguments!");
  return main_func({const_cast<const char**>(argv), static_cast<size_t>(argc)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
