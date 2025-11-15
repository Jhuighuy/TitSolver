/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <mutex>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <execinfo.h>
#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/build_info.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/print.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stacktrace.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys_info.hpp"
#include "tit/core/type.hpp"

#ifdef TIT_HAVE_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ewrite(std::string_view message) noexcept {
  static_cast<void>(write(STDERR_FILENO, message.data(), message.size()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void eprintln_crash_report(
    std::string_view message,
    std::string_view cause = "",
    std::string_view cause_description = "",
    std::source_location loc = std::source_location::current(),
    Stacktrace stacktrace = Stacktrace::current()) {
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
  eprintln("{}", stacktrace);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void println_logo_and_system_info() {
  constexpr auto logo_lines = std::to_array<std::string_view>({
      R"(               ############               )",
      R"(          ######################          )",
      R"(        #######            #######        )",
      R"(      ######                  ######      )",
      R"(    #####          _,########._  #####    )",
      R"(   #####         .##############. #####   )",
      R"(  #####        .####"__'#########. #####  )",
      R"(  ####        _#### |_'| ##########.####  )",
      R"( ####      _-"``\"  `--  """'  `###; #### )",
      R"( ####     "--==="#.             `###.#### )",
      R"( ####          "###.         __.######### )",
      R"( ####           `####._ _.=######" "##### )",
      R"(  ####           ############"      ####  )",
      R"(  #####          #######'          #####  )",
      R"(   #####         #####'           #####   )",
      R"(    #####        `###'          #####     )",
      R"(      ######      `##         ######      )",
      R"(        #######    `#.     #######        )",
      R"(          ######################          )",
      R"(               ############               )",
  });

  /// @todo In C++26 there would be no need for `std::string{...}`.
  std::chrono::year_month_day commit_date{};
  std::istringstream{std::string{build_info::commit_date()}} >>
      std::chrono::parse("%F", commit_date);

  std::vector<std::string> info_lines{
      "BlueTit Solver",
      "",
      std::format("© 2020 - {:%Y} Oleg Butakov", commit_date),
      "",
      std::format("Version ........ {}", build_info::version()),
      std::format("Commit ......... {}", build_info::commit_hash()),
  };

  try {
    info_lines.push_back(
        std::format("Host ........... {}", sys_info::host_name()));
  } catch (const Exception& e) {
    err("Unable to get host name: {}.", e.what());
  }

  try {
    info_lines.push_back(
        std::format("OS ............. {}", sys_info::os_info()));
  } catch (const Exception& e) {
    err("Unable to get OS information: {}.", e.what());
  }

  try {
    info_lines.push_back(
        std::format("CPU ............ {}", sys_info::cpu_info()));
  } catch (const Exception& e) {
    err("Unable to get CPU information: {}.", e.what());
  }

  try {
    info_lines.push_back(
        std::format("RAM ............ {}", fmt_memsize(sys_info::ram_size())));
  } catch (const Exception& e) {
    err("Unable to get RAM size: {}.", e.what());
  }

  const auto current_path = std::filesystem::current_path();
  try {
    /// @todo In C++26 there would be no need for `.string()`.
    info_lines.push_back(
        std::format("Work Dir ....... {}", current_path.string()));
  } catch (const Exception& e) {
    err("Unable to get working directory: {}.", e.what());
  }

  try {
    info_lines.push_back(std::format(
        "Disk space ..... {}",
        fmt_memsize(std::filesystem::space(current_path).available)));
  } catch (const Exception& e) {
    err("Unable to get disk space: {}.", e.what());
  }

  TIT_ASSERT(info_lines.size() <= logo_lines.size(), "Too many lines!");
  const auto padding = (logo_lines.size() - info_lines.size()) / 2;
  info_lines.resize(logo_lines.size());
  /// @todo `std::ranges::shift_right` is not available in libstdc++ yet.
  std::shift_right(info_lines.begin(), // NOLINT(modernize-use-ranges)
                   info_lines.end(),
                   static_cast<ssize_t>(padding));

  println();
  println_separator('~');
  println();
  for (const auto& [logo_line, info_line] :
       std::views::zip(logo_lines, info_lines) | std::views::as_const) {
    println("{}   {}", logo_line, info_line);
  }
  println();
  println_separator('~');
  println();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto crash_report_mutex() -> std::recursive_mutex& {
  static std::recursive_mutex mutex;
  return mutex;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup_signal_handlers() noexcept {
  // Preload `libgcc` beforehand to increase the chances of `backtrace` being
  // safe to call from a signal handler.
  void* dummy_trace = nullptr; // NOLINT(*-const-correctness)
  backtrace(&dummy_trace, 1);

  // Define descriptions for a subset of signals.
  // NOLINTBEGIN(*-include-cleaner) -- Some of these are not in C++ standard.
  using SigDescr = std::pair<int, std::string_view>;
  static constexpr auto signals = std::to_array<SigDescr>({
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
      const std::scoped_lock lock{crash_report_mutex()};

      // Report the signal.
      ewrite("\n");
      ewrite("\n");
      const auto* const iter = std::ranges::find_if( //
          signals,
          [sig](const auto& sig_descr) { return sig_descr.first == sig; });
      ewrite(iter != signals.end() ? iter->second : "Unknown signal.\n");

      // Print the stack trace, if needed, and exit.
      if (sig == SIGINT || sig == SIGTERM) {
        std::exit(-sig); // NOLINT(concurrency-mt-unsafe)
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
        fast_exit(-sig);
      }
    });
    if (prev_handler == SIG_ERR) {
      terminate_on_exception([descr] { // NOLINTNEXTLINE(*-mt-unsafe)
        err("Unable to set handler for '{}': {}.", descr, std::strerror(errno));
      });
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup_terminate_handler() noexcept {
  static std::terminate_handler default_terminate_handler = nullptr;
  default_terminate_handler = std::set_terminate([] [[noreturn]] noexcept {
    const std::scoped_lock lock{crash_report_mutex()};

    // Get the current exception, if any, and re-throw it.
    if (const auto exception_ptr = std::current_exception(); exception_ptr) {
      try {
        std::rethrow_exception(exception_ptr);
      } catch (const Exception& e) {
        terminate_on_exception([&e] {
          eprintln_crash_report(
              "Terminating due to an unhandled exception.",
              std::format("throw {}{{...}};", type_name_of(e)),
              e.what(),
              e.where(),
              e.when());
        });
      } catch (const std::exception& e) {
        terminate_on_exception([&e] {
          eprintln_crash_report(
              "Terminating due to an unhandled exception.",
              std::format("throw {}{{...}};", type_name_of(e)),
              e.what());
        });
      } catch (...) {
        terminate_on_exception([] {
          eprintln_crash_report("Terminating due to an unhandled exception.");
        });

        // Default handler should provide more information. It will likely
        // raise `SIGABRT`, so we should replace our fancy handler with a
        // simple one beforehand.
        if (default_terminate_handler != nullptr) {
          static_cast<void>(std::signal(SIGABRT, [] [[noreturn]] (int /*sig*/) {
            fast_exit(1);
          }));
          default_terminate_handler();
          std::unreachable(); // Should not return.
        }
      }
    } else {
      terminate_on_exception([] {
        eprintln_crash_report("Terminating due to a call to std::terminate().");
      });
    }

    // Since we consider this a crash, let's not invoke at-exit handlers.
    fast_exit(1);
  });
  if (default_terminate_handler == nullptr) {
    terminate_on_exception([] { //
      err("Unable to register the terminate handler.");
    });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

// Note: Declared in `checks.hpp`.
[[noreturn]] void checks::report_assert_failure(
    std::string_view expression,
    std::string_view message,
    std::source_location location) noexcept {
  const std::scoped_lock lock{crash_report_mutex()};

  // Report the assertion failure.
  terminate_on_exception([expression, message, location] {
    eprintln_crash_report("Internal consistency check failed!",
                          expression,
                          message,
                          location);
  });

  // Exit the process.
  fast_exit(1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_main(int argc,
              char** argv,
              std::move_only_function<void(int, char**)> main) noexcept -> int {
  // Setup error handlers.
  setup_signal_handlers();
  setup_terminate_handler();

  // Handlers are set up now, run the main function inside a safe block.
  terminate_on_exception([argc, argv, &main] {
    // Print the logo and system information. Skip the logo if requested. If
    // logo is printed, set the variable to prevent printing it again in the
    // child processes.
    if (!get_env("TIT_NO_BANNER", false)) {
      println_logo_and_system_info();
      set_env("TIT_NO_BANNER", true);
    }

    // Enable subsystems.
    if (get_env("TIT_ENABLE_STATS", false)) Stats::enable();
    if (get_env("TIT_ENABLE_PROFILER", false)) Profiler::enable();

    // Run the main function.
    main(argc, argv);
  });

  return 0;
}

auto run_main(int argc,
              char** argv,
              std::move_only_function<void()> main) noexcept -> int {
  return run_main(argc, argv, [&main](int /*argc*/, char** /*argv*/) {
    main();
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[noreturn]] void fast_exit(int exit_code) noexcept {
#ifdef TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(exit_code);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
