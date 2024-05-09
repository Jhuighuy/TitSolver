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
  eprint("\n\n{}:{}:{}: Internal consistency check failed!\n\n",
         location.file_name(),
         location.line(),
         location.column());
  eprint("  {}\n", expression);
  eprint("  ^{:~>{}} {}\n\n", "", expression.size() - 1, message);
  std::abort();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
