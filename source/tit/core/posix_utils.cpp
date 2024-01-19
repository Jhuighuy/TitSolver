/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <ranges>
#include <string_view>
#include <vector>

#include <execinfo.h>
#include <signal.h> // NOLINT(*-deprecated-headers)
#include <stdio.h>  // NOLINT(*-deprecated-headers)
#include <sys/ioctl.h>
#ifdef __APPLE__
#include <sys/signal.h>
#include <sys/ttycom.h>
#endif
#include <unistd.h>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"
#include "tit/core/posix_utils.hpp"
#include "tit/core/types.hpp"

#if TIT_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Exit from the current process. */
[[noreturn]] void exit(int exit_code) noexcept {
  std::exit(exit_code); // NOLINT(concurrency-mt-unsafe)
}

/** Fast-exit from the current process. */
[[noreturn]] void fast_exit(int exit_code) noexcept {
#if TIT_GCOV
  __gcov_dump();
#endif
  std::_Exit(exit_code);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::vector<SignalHandler*> SignalHandler::handlers_{};

SignalHandler::SignalHandler(std::initializer_list<int> signal_numbers) {
  // Register the current handler object.
  handlers_.push_back(this);
  // Register the new signal actions (or handlers).
  prev_actions_.reserve(signal_numbers.size());
  for (const auto signal_number : signal_numbers) {
    TIT_ENSURE(signal_number < NSIG, "Signal number is out of range!");
    // Prepare the new action.
    sigaction_t action{};
    action.sa_flags = 0;
    action.sa_handler = &handle_signal_;
    sigemptyset(&action.sa_mask);
    // Register the new action and store the previous one.
    sigaction_t prev_action{};
    const auto status = sigaction(signal_number, &action, &prev_action);
    TIT_ENSURE(status == 0, "Unable to set the signal action!");
    prev_actions_.emplace_back(signal_number, prev_action);
  }
}

SignalHandler::~SignalHandler() noexcept {
  // Restore the old signal handlers or actions.
  for (const auto& [signal_number, prev_action] : prev_actions_) {
    const auto status = sigaction(signal_number, &prev_action, nullptr);
    TIT_ENSURE(status == 0, "Unable to reset the signal action!");
  }
  // Unregister the current signal handler.
  TIT_ASSERT(handlers_.back() == this, "Signal handler was not registered!");
  handlers_.pop_back();
}

void SignalHandler::handle_signal_(int signal_number) noexcept {
  // Traverse the registered handler and find the one that handles the signal
  // that we've just got.
  for (auto* const handler : handlers_ | std::views::reverse) {
    TIT_ASSERT(handler != nullptr, "Invalid handler was registered.");
    const auto iter = std::ranges::find(handler->signals(), signal_number);
    if (iter != handler->signals().end()) {
      handler->on_signal(signal_number);
      return;
    }
  }
  TIT_ASSERT(false, "Interpected a signal that has no handler!");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace {

// Dump a message in the "async-signal-safe" way.
void dump(std::string_view message) noexcept {
  write(STDERR_FILENO, message.data(), message.size());
}

// Dump backtrace in the "async-signal-safe" way.
[[gnu::always_inline]] inline void dump_backtrace() noexcept {
  constexpr int max_stack_depth = 100;
  std::array<void*, max_stack_depth> stack_trace{};
  const auto stack_depth = backtrace(stack_trace.data(), max_stack_depth);
  backtrace_symbols_fd(stack_trace.data(), stack_depth, STDERR_FILENO);
}

} // namespace

FatalSignalHandler::FatalSignalHandler()
    : SignalHandler{SIGINT, SIGHUP,  SIGQUIT, SIGILL,  SIGTRAP, SIGABRT, SIGFPE,
                    SIGBUS, SIGSEGV, SIGSYS,  SIGPIPE, SIGALRM, SIGTERM} {}

void FatalSignalHandler::on_signal(int signal_number) noexcept {
  if (signal_number == SIGINT) {
    // Exit normally.
    dump("\n\nInterrupted by Ctrl+C.\n");
    exit(0);
  } else {
    // Dump backtrace and exit fast with an error.
    dump("\n\nTerminated by signal ");
    switch (signal_number) {
      case SIGHUP:  dump("SIGHUP (hangup)"); break;
      case SIGQUIT: dump("SIGQUIT (quit)"); break;
      case SIGILL:  dump("SIGILL (illegal instruction)"); break;
      case SIGTRAP: dump("SIGTRAP (trace trap)"); break;
      case SIGABRT: dump("SIGABRT (aborted)"); break;
      case SIGFPE:  dump("SIGFPE (floating-point exception)"); break;
      case SIGBUS:  dump("SIGBUS (bus error)"); break;
      case SIGSEGV: dump("SIGSEGV (segmentation fault)"); break;
      case SIGSYS:  dump("SIGSYS (bad system call)"); break;
      case SIGPIPE: dump("SIGPIPE (broken pipe)"); break;
      case SIGALRM: dump("SIGALRM (alarm clock)"); break;
      case SIGTERM: dump("SIGTERM"); break;
      default:      TIT_ASSERT(false, "Must not be reached.");
    }
    dump(".\n");
    dump("\nStacktrace:\n");
    dump_backtrace();
    fast_exit(1);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto tty_width(std::FILE* stream) noexcept -> size_t {
  const auto stream_fileno = fileno(stream);
  if (isatty(stream_fileno) == 0) return 80; // Redirected.
  struct winsize window_size = {};
  // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(stream_fileno, TIOCGWINSZ, &window_size);
  TIT_ENSURE(status == 0, "Unable to query terminal window size!");
  return window_size.ws_col;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
