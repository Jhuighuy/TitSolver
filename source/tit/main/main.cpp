/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <format>
#include <mutex>
#include <source_location>
#include <stacktrace>
#include <string_view>
#include <utility>

#include <execinfo.h>
#include <unistd.h>

#include "tit/core/exception.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/print.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/type.hpp"

#include "tit/main/main.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ewrite(std::string_view message) noexcept {
  static_cast<void>(write(STDERR_FILENO, message.data(), message.size()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @todo There is a code duplication between this function and
///       `report_check_failure` in `checks.cpp`. We should refactor it.
void eprintln_crash_report(
    std::string_view message,
    std::string_view cause = "",
    std::string_view cause_description = "",
    std::source_location loc = std::source_location::current(),
    std::stacktrace trace = std::stacktrace::current()) {
  eprintln();
  eprintln();
  eprint("{}:{}:{}: {}", loc.file_name(), loc.line(), loc.column(), message);

  if (!cause.empty()) {
    eprintln();
    eprintln();
    eprintln("  {}", cause);
    if (!cause_description.empty()) {
      eprintln("  ^{:~>{}} {}", "", cause.size() - 1, cause_description);
    }
  }

  eprintln();
  eprintln();
  eprintln("Stack trace:");
  eprintln();
  eprintln("{}", trace);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto crash_report_mutex() -> std::recursive_mutex& {
  static std::recursive_mutex mutex;
  return mutex;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup_signal_handlers() {
  // Preload `libgcc` beforehand to increase the chances of `backtrace` being
  // safe to call from a signal handler.
  void* dummy_trace = nullptr;
  backtrace(&dummy_trace, 1);

  // Define descriptions for a subset of signals.
  // NOLINTBEGIN(*-include-cleaner) -- Some of these are not in C++ standard.
  static constexpr auto signals = std::to_array<std::pair<int, CStrView>>({
      {SIGHUP, "Hangup (SIGHUP).\n"},
      {SIGINT, "Interrupted by Ctrl+C (SIGINT).\n"},
      {SIGQUIT, "Quit (SIGQUIT).\n"},
      {SIGILL, "Illegal instruction (SIGILL).\n"},
      {SIGTRAP, "Trace / breakpoint trap (SIGTRAP).\n"},
      {SIGABRT, "Aborted (SIGABRT).\n"},
      {SIGFPE, "Floating-point exception (SIGFPE).\n"},
      {SIGBUS, "Bus error (SIGBUS).\n"},
      {SIGSEGV, "Segmentation fault (SIGSEGV).\n"},
      {SIGSYS, "Bad system call (SIGSYS).\n"},
      {SIGPIPE, "Broken pipe (SIGPIPE).\n"},
      {SIGALRM, "Alarm clock (SIGALRM).\n"},
      {SIGTERM, "Terminated by signal (SIGTERM).\n"},
  });
  // NOLINTEND(*-include-cleaner)

  // Setup the signal handlers.
  for (const auto& [signum, descr] : signals) {
    const auto prev_handler = std::signal(signum, [] [[noreturn]] (int sig) {
      const std::lock_guard lock{crash_report_mutex()};

      // Report the signal.
      ewrite("\n");
      ewrite("\n");
      const auto* const iter = std::ranges::find_if( //
          signals,
          [sig](const auto& sig_descr) { return sig_descr.first == sig; });
      ewrite(iter != signals.end() ? iter->second : "Unknown signal.\n");

      // Print the stack trace, if needed, and exit.
      if (sig == SIGINT || sig == SIGTERM) {
        exit(static_cast<ExitCode>(-sig));
      } else {
        ewrite("\n");
        ewrite("\n");
        ewrite("Stack trace:\n");
        ewrite("\n");
        constexpr int max_depth = 1000;
        std::array<void*, max_depth> trace{};
        const auto depth = backtrace(trace.data(), max_depth);
        backtrace_symbols_fd(trace.data(), depth, STDERR_FILENO);

        // Since we consider this a crash, let's not invoke at-exit handlers.
        fast_exit(static_cast<ExitCode>(-sig));
      }
    });
    if (prev_handler == SIG_ERR) { // NOLINTNEXTLINE(*-mt-unsafe)
      err("Unable to set handler for '{}': {}.", descr, std::strerror(errno));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup_terminate_handler() {
  static std::terminate_handler default_terminate_handler = nullptr;
  default_terminate_handler = std::set_terminate([] [[noreturn]] {
    const std::lock_guard lock{crash_report_mutex()};

    // Get the current exception, if any, and re-throw it.
    if (const auto exception_ptr = std::current_exception(); exception_ptr) {
      try {
        std::rethrow_exception(exception_ptr);
      } catch (const Exception& e) {
        eprintln_crash_report("Terminating due to an unhandled exception.",
                              std::format("throw {}{{...}};", type_name_of(e)),
                              e.what(),
                              e.where(),
                              e.when());
      } catch (const std::exception& e) {
        eprintln_crash_report("Terminating due to an unhandled exception.",
                              std::format("throw {}{{...}};", type_name_of(e)),
                              e.what());
      } catch (...) {
        // Default handler should provide more information. It will likely
        // raise `SIGABRT`, so we should replace our fancy handler with a
        // simple one beforehand.
        eprintln_crash_report("Terminating due to an unhandled exception.");
        if (default_terminate_handler != nullptr) {
          static_cast<void>(std::signal(SIGABRT, [] [[noreturn]] (int /*sig*/) {
            fast_exit(ExitCode{1});
          }));
          default_terminate_handler();
          std::unreachable(); // Should not return.
        }
      }
    } else {
      eprintln_crash_report("Terminating due to a call to std::terminate().");
    }

    // Since we consider this a crash, let's not invoke at-exit handlers.
    fast_exit(ExitCode{1});
  });
  if (default_terminate_handler == nullptr) {
    err("Unable to register the terminate handler.");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

auto main(int argc, char** argv) noexcept(false) -> int {
  using namespace tit;

  // Setup error handlers.
  setup_signal_handlers();
  setup_terminate_handler();

  // Enable subsystems.
  if (get_env("TIT_ENABLE_STATS", false)) Stats::enable();
  if (get_env("TIT_ENABLE_PROFILER", false)) Profiler::enable();

  // Setup parallelism.
  par::set_num_threads(get_env("TIT_NUM_THREADS", 8UZ));

  // Run the main function.
  tit_main({argc, argv});
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
