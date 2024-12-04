/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __APPLE__
#include <array>
#endif
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/ttycom.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>

#include <boost/core/demangle.hpp>

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
  if (status != 0) TIT_THROW("Unable to register at-exit callback!");
}

[[noreturn]]
void exit(ExitCode exit_code) noexcept {
  std::exit(std::to_underlying(exit_code)); // NOLINT(concurrency-mt-unsafe)
}

[[noreturn]]
void fast_exit(ExitCode exit_code) noexcept {
#ifdef TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(std::to_underlying(exit_code));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_system(const char* command) {
  // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
  const auto status = std::system(command);
  static_cast<void>(status); /// @todo Ignore the status code for now.
  // if (status != 0) TIT_THROW("System command '{}' failed!", command);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto exe_path() -> std::filesystem::path {
#ifdef __APPLE__
  std::array<char, PROC_PIDPATHINFO_MAXSIZE> buffer{};
  const auto status = proc_pidpath(getpid(), buffer.data(), sizeof(buffer));
  if (status <= 0) TIT_THROW("Unable to query the current executable path!");
  return buffer.data();
#elifdef __linux__
  return std::filesystem::canonical("/proc/self/exe");
#else
#error Unsupported platform!
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tty_width(TTY tty) -> std::optional<size_t> {
  const auto tty_fileno = std::to_underlying(tty);
  if (isatty(tty_fileno) == 0) return std::nullopt; // Redirected.

  struct winsize window_size = {};
  // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(tty_fileno, TIOCGWINSZ, &window_size);
  if (status != 0) {
    TIT_THROW("Unable to query terminal window size with fileno {}!",
              tty_fileno);
  }
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
