/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <csignal>
#include <exception>
#include <format>
#include <ranges>
#include <string_view>
#include <utility>

#include <execinfo.h>
#include <unistd.h>

#include "tit/core/env.hpp"
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

void setup_signal_handlers() noexcept {
  static constexpr auto ewrite = [](std::string_view message) noexcept {
    std::ignore = write(STDERR_FILENO, message.data(), message.size());
  };

  std::ignore = std::signal(SIGINT, [] [[noreturn]] (int /*sig*/) {
    ewrite("\n");
    ewrite("\n");
    ewrite("Interrupted by Ctrl+C.\n");
    exit(ExitCode{-SIGINT});
  });
  std::ignore = std::signal(SIGTERM, [] [[noreturn]] (int /*sig*/) {
    ewrite("\n");
    ewrite("\n");
    ewrite("Terminated by signal.\n");
    exit(ExitCode{-SIGTERM});
  });

  // Preload `libgcc` beforehand to increase the chances of `backtrace` being
  // safe to call from a signal handler.
  void* dummy_trace = nullptr;
  backtrace(&dummy_trace, 1);

  // NOLINTBEGIN(*-include-cleaner) -- Some of these are not in C++ standard.
  static constexpr auto signals = std::to_array<std::pair<int, CStrView>>({
      {SIGHUP, "Hangup (SIGHUP).\n"},
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
  });
  // NOLINTEND(*-include-cleaner)
  for (const auto sig : signals | std::views::keys) {
    std::ignore = std::signal(sig, [] [[noreturn]] (int sig_) {
      ewrite("\n");
      ewrite("\n");
      const auto* const iter = std::ranges::find_if( //
          signals,
          [sig_](const auto& sig_descr) { return sig_descr.first == sig_; });
      ewrite(iter != signals.end() ? iter->second : "Unknown signal.\n");

      ewrite("\n");
      ewrite("\n");
      ewrite("Stack trace:\n");
      ewrite("\n");
      constexpr int max_depth = 1000;
      std::array<void*, max_depth> trace{};
      const auto depth = backtrace(trace.data(), max_depth);
      backtrace_symbols_fd(trace.data(), depth, STDERR_FILENO);

      fast_exit(static_cast<ExitCode>(-sig_));
    });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup_terminate_handler() {
  static std::terminate_handler default_terminate_handler = nullptr;
  default_terminate_handler = std::set_terminate([] [[noreturn]] {
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
        eprintln_crash_report("Terminating due to an unhandled exception.");
        if (default_terminate_handler != nullptr) {
          std::ignore = std::signal(SIGABRT, SIG_DFL);
          default_terminate_handler(); // Should give us more information.
        }
      }
    } else {
      eprintln_crash_report("Terminating due to a call to std::terminate().");
    }
    fast_exit(ExitCode::failure);
  });
  if (default_terminate_handler == nullptr) {
    error("Unable to register the terminate handler.");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

auto main(int argc, char** argv) -> int { // NOLINT(*-exception-escape)
  using namespace tit;

  // Setup error handlers.
  setup_signal_handlers();
  setup_terminate_handler();

  // Print the logo and system information.
  // Skip the logo if requested. If logo is printed, set the variable to
  // prevent printing it again in the child processes.
  if (!get_env("TIT_NO_BANNER", false)) println_logo_and_system_info();
  set_env("TIT_NO_BANNER", true);

  // Enable subsystems.
  if (get_env("TIT_ENABLE_STATS", false)) Stats::enable();
  if (get_env("TIT_ENABLE_PROFILER", false)) Profiler::enable();

  // Setup parallelism.
  par::set_num_threads(get_env("TIT_NUM_THREADS", cpu_perf_cores()));

  // Run the main function.
  main({argc, argv});
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
