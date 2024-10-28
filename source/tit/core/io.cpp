/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>

#include "tit/core/io.hpp"
#include "tit/core/missing.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void eprint_stacktrace(const Std::stacktrace& stacktrace) {
  if (!stacktrace) return;
  eprintln();
  eprintln("Stack trace:");
  eprintln();
  for (const auto& [index, frame] : std::views::enumerate(stacktrace)) {
    eprintln("{:>3} {} {}", index, frame.address(), frame.name());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Required on MacOS Sequoia.
#if defined(__APPLE__) && defined(_LIBCPP_VERSION)
// NOLINTBEGIN
_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_EXPORTED_FROM_ABI auto __is_posix_terminal(FILE* __stream) -> bool {
  return isatty(fileno(__stream));
}
_LIBCPP_EXPORTED_FROM_ABI auto __get_ostream_file(ostream& /*__os*/) -> FILE* {
  return nullptr;
}
_LIBCPP_END_NAMESPACE_STD
// NOLINTEND
#endif
