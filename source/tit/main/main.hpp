/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/checks.hpp"

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

/// Entry point for the command line application forward declaration.
///
/// Actual `main` function is defined in the `tit/core/cmd.cpp` file. It sets up
// the environment: initialized threading, error handlers, etc and calls the
/// `tit::main` function.
void main(CmdArgs args);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
