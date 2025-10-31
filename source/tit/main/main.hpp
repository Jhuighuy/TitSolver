/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
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

/// Forward declaration of the entry point for the application.
void tit_main(tit::CmdArgs args);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
