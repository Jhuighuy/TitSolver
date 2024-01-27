/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>

#include "tit/core/checks.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/system_utils.hpp"
#include "tit/par/thread.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto run_main(int argc, char** argv, main_like_t main_func) noexcept -> int {
  TIT_ENSURE(main_func != nullptr, "Main function must be specified!");
  // Setup signal handler.
  FatalSignalHandler const handler{};
  // Enable profiling.
  if (std::getenv("TIT_ENABLE_PROFILER") != nullptr) { // NOLINT(*-mt-unsafe)
    Profiler::enable();
  }
  // Setup parallelism and run the main function.
  return par::main(argc, argv, [main_func](int argc_, char** argv_) {
    return main_func(argc_, argv_);
  });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
