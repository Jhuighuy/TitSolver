/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <span>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Command line arguments.
using CmdArgs = std::span<const char*>;

/// `main`-like function pointer.
using MainFunc = std::function<int(CmdArgs)>;

/// Wrapper for the main function that should run sets up the
/// environment: initialized threading, error handlers, etc.
auto run_main(int argc, char** argv, const MainFunc& main_func) -> int;

/// Implement the actual `main` function.
#define TIT_IMPLEMENT_MAIN(main_func)                                          \
  auto main(int argc, char** argv) -> int {                                    \
    using namespace tit;                                                       \
    return run_main(argc, argv, &(main_func));                                 \
  }

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
