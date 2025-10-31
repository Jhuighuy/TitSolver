/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <source_location>
#include <string_view>

#include "tit/core/checks.hpp"
#include "tit/core/print.hpp"
#include "tit/core/stacktrace.hpp"

namespace tit::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[noreturn]] void report_check_failure( // NOLINT(*-exception-escape)
    std::string_view expression,
    std::string_view message,
    std::source_location location) noexcept {
  // Report the check failure.
  eprintln();
  eprintln();
  eprintln("{}:{}:{}: Internal consistency check failed!",
           location.file_name(),
           location.line(),
           location.column());
  eprintln();
  eprintln("  {}", expression);
  eprintln("  ^{:~>{}} {}", "", expression.size() - 1, message);
  eprintln();
  eprintln();
  eprintln("Stack trace:");
  eprintln();
  eprintln("{}", Stacktrace::current());

  // Abort execution.
  std::abort();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
