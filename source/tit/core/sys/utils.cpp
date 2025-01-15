/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
#include "tit/core/str_utils.hpp"
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

auto exe_path() -> std::filesystem::path {
#ifdef __APPLE__
  std::array<char, PROC_PIDPATHINFO_MAXSIZE> buffer{};
  if (proc_pidpath(getpid(), buffer.data(), sizeof(buffer)) <= 0) {
    TIT_THROW("Unable to query the current executable path!");
  }
  return buffer.data();
#elifdef __linux__
  return std::filesystem::canonical("/proc/self/exe");
#else
#error Unsupported platform!
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto get_env(CStrView name) noexcept -> std::optional<std::string_view> {
  const auto* const value = std::getenv(name.c_str()); // NOLINT(*-mt-unsafe)
  if (value == nullptr) return std::nullopt;
  return std::string_view{value};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tty_width(TTY tty) -> std::optional<size_t> {
  const auto tty_fileno = std::to_underlying(tty);
  if (isatty(tty_fileno) == 0) return std::nullopt; // Redirected.

  winsize window_size = {}; // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  if (ioctl(tty_fileno, TIOCGWINSZ, &window_size) != 0) {
    TIT_THROW("Unable to query terminal window size with fileno {}!",
              tty_fileno);
  }
  return window_size.ws_col;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto try_demangle(CStrView mangled_name) -> std::optional<std::string> {
  const boost::core::scoped_demangled_name demangled_name{mangled_name.c_str()};
  if (const auto* p = demangled_name.get(); p != nullptr) return p;
  return std::nullopt;
}

auto maybe_demangle(CStrView mangled_name) -> std::string {
  return try_demangle(mangled_name).value_or(mangled_name.c_str());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
