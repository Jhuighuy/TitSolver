/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

#include <sys/ioctl.h>
#ifdef __APPLE__
#include <sys/ttycom.h>
#endif
#include <unistd.h>

#include <boost/core/demangle.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/sys_utils.hpp"

#ifndef TIT_HAVE_GCOV
#define TIT_HAVE_GCOV 0
#endif

#if TIT_HAVE_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_atexit(atexit_callback_t callback) noexcept {
  TIT_ASSERT(callback != nullptr, "At-exit callback is invalid!");
  const auto status = std::atexit(callback);
  TIT_ENSURE(status == 0, "Unable to register at-exit callback!");
}

[[noreturn]]
void exit(ExitCode exit_code) noexcept {
  std::exit(std::to_underlying(exit_code)); // NOLINT(concurrency-mt-unsafe)
}

[[noreturn]]
void fast_exit(ExitCode exit_code) noexcept {
#if TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(std::to_underlying(exit_code));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tty_width(TTY tty) noexcept -> std::optional<size_t> {
  const auto tty_fileno = static_cast<int>(tty);
  if (isatty(tty_fileno) == 0) return std::nullopt; // Redirected.

  struct winsize window_size = {};
  // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(tty_fileno, TIOCGWINSZ, &window_size);
  TIT_ENSURE(status == 0, "Unable to query terminal window size!");
  return window_size.ws_col;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto try_demangle(const char* mangled_name) -> std::optional<std::string> {
  const boost::core::scoped_demangled_name demangled_name{mangled_name};
  if (const auto* p = demangled_name.get(); p != nullptr) return p;
  return std::nullopt;
}

auto maybe_demangle(const char* mangled_name) -> std::string {
  return try_demangle(mangled_name).value_or(mangled_name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
