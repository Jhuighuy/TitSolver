/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>

#include "tit/core/checks.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Command line arguments.
class CmdArgs final {
public:

  /// Construct the command line arguments.
  constexpr CmdArgs(int argc, char** argv) : argc_{argc}, argv_{argv} {
    TIT_ASSERT(argc >= 1, "Invalid number of command line arguments!");
    TIT_ASSERT(argv != nullptr, "Invalid command line arguments!");
  }

  /// Get the number of command line arguments.
  constexpr auto argc() const noexcept -> int {
    return argc_;
  }

  /// Get the command line argument values.
  constexpr auto argv() const noexcept -> char** {
    return argv_;
  }

private:

  int argc_;
  char** argv_;

}; // class CmdArgs

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Main function pointer.
using MainFunc = std::move_only_function<int(CmdArgs)>;

/// Wrapper for the main function that should run sets up the
/// environment: initialized threading, error handlers, etc.
auto run_main(int argc, char** argv, MainFunc main_func) -> int;

/// Implement the actual `main` function.
#define TIT_IMPLEMENT_MAIN(main_func)                                          \
  auto main(int argc, char** argv) -> int {                                    \
    using namespace tit;                                                       \
    return run_main(argc, argv, &(main_func));                                 \
  }

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
