/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <source_location>
#include <string_view>

#include "tit/core/checks.hpp"
#include "tit/core/io.hpp"

namespace tit::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[noreturn]]
void handle_check_failure( // NOLINT(*-exception-escape)
    std::string_view expression,
    std::string_view message,
    std::source_location location) noexcept {
  /// @todo We should also report stacktrace once we have it.
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

  std::abort();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
