/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <optional>
#include <utility>

#ifdef __APPLE__
#include <sys/ttycom.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys/utils.hpp"

#ifdef TIT_HAVE_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_atexit(atexit_callback_t callback) {
  TIT_ASSERT(callback != nullptr, "At-exit callback is invalid!");
  const auto status = std::atexit(callback);
  TIT_ENSURE(status == 0, "Unable to register at-exit callback!");
}

[[noreturn]] void exit(ExitCode exit_code) noexcept {
  std::exit(std::to_underlying(exit_code)); // NOLINT(concurrency-mt-unsafe)
}

[[noreturn]] void fast_exit(ExitCode exit_code) noexcept {
#ifdef TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(std::to_underlying(exit_code));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tty_width(TTY tty) -> std::optional<size_t> {
  const auto tty_fileno = std::to_underlying(tty);
  if (isatty(tty_fileno) == 0) return std::nullopt; // Redirected.

  winsize window_size = {}; // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(tty_fileno, TIOCGWINSZ, &window_size);
  TIT_ENSURE(status == 0,
             "Unable to query terminal window size with fileno {}!",
             tty_fileno);
  return window_size.ws_col;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
