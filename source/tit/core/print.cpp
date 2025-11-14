/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __APPLE__
#include <sys/ttycom.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

constexpr auto default_terminal_width = 80;

auto terminal_width(int fd) -> size_t {
  if (isatty(fd) == 0) return default_terminal_width;

  winsize window_size = {}; // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(fd, TIOCGWINSZ, &window_size);
  TIT_ENSURE_ERRNO(status == 0,
                   "Unable to query terminal window size with fileno {}!",
                   fd);

  return window_size.ws_col;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void println_separator(char c) {
  for (size_t i = 0; i < terminal_width(STDOUT_FILENO); ++i) print("{}", c);
  print("\n");
}

void eprintln_separator(char c) {
  for (size_t i = 0; i < terminal_width(STDERR_FILENO); ++i) eprint("{}", c);
  eprint("\n");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
