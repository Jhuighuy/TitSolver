/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/checks.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/exception.hpp"
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

  // Enable subsystems.
  if (get_env("TIT_ENABLE_STATS", false)) Stats::enable();
  if (get_env("TIT_ENABLE_PROFILER", false)) Profiler::enable();

  // Setup parallelism.
  par::set_num_threads(get_env("TIT_NUM_THREADS", 8UZ));

  // Run the main function.
  TIT_ASSERT(main_func != nullptr, "Main function must be specified!");
  return main_func({argc, argv});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
