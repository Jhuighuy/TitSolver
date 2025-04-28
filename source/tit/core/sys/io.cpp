/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <utility>

#ifdef __APPLE__
#include <sys/ttycom.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys/io.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto is_terminal(fd_t fd) -> bool {
  return isatty(std::to_underlying(fd)) != 0;
}

auto terminal_width(fd_t fd) -> size_t {
  if (!is_terminal(fd)) return default_terminal_width;

  winsize window_size = {}; // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(std::to_underlying(fd), TIOCGWINSZ, &window_size);
  TIT_ENSURE(status == 0,
             "Unable to query terminal window size with fileno {}!",
             std::to_underlying(fd));

  return window_size.ws_col;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
