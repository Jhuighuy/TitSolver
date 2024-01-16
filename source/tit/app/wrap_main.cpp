/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/app/wrap_main.hpp"
#include "tit/core/posix_utils.hpp"
#include "tit/par/thread.hpp"

namespace tit::app {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto wrap_main(int argc, char** argv, main_like_t main_func) noexcept -> int {
  // A simple implementation for now.
  const FatalSignalHandler handler{};
  return par::main(argc, argv, [main_func](int the_argc, char** the_argv) {
    return main_func(the_argc, the_argv);
  });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::app
