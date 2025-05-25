/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <source_location>
#include <string_view>

#include "tit/core/checks.hpp"
#include "tit/core/io.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/sys/stacktrace.hpp"
#include "tit/core/sys/utils.hpp"

namespace tit::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[noreturn]] void report_check_failure( // NOLINT(*-exception-escape)
    std::string_view expression,
    std::string_view message,
    std::source_location location) noexcept {
  const par::GlobalLock lock{};

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
  eprintln("{}", Stacktrace::current());

  // Fast-exit with failure.
  fast_exit(ExitCode::failure);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
