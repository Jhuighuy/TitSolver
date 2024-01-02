/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <csignal>
#include <format>
#include <initializer_list>
#include <ranges>
#include <vector>

#include <execinfo.h>
#include <unistd.h>

#include "tit/core/assert.hpp"
#include "tit/core/misc.hpp"
#include "tit/core/signals.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
std::vector<SignalHandler*> SignalHandler::handlers_{};

SignalHandler::SignalHandler()
    : SignalHandler({SIGINT, SIGTERM, SIGABRT, SIGSEGV, SIGILL, SIGFPE}) {}

SignalHandler::SignalHandler(std::initializer_list<int> signal_numbers) {
  // Register the current handler object.
  handlers_.push_back(this);
  // Register the new signal actions (or handlers).
  prev_actions_.reserve(signal_numbers.size());
  for (const auto signal_number : signal_numbers) {
    // Prepare the new action.
    // NOLINTBEGIN(*-include-cleaner)
    sigaction_t action{};
    action.sa_flags = 0;
    action.sa_handler = &handle_signal_;
    sigemptyset(&action.sa_mask);
    // Register the new action and store the previous one.
    sigaction_t prev_action{};
    const auto status = sigaction(signal_number, &action, &prev_action);
    TIT_ENSURE(status == 0, "Unable to set the signal action!");
    // NOLINTEND(*-include-cleaner)
    prev_actions_.emplace_back(signal_number, prev_action);
  }
}

SignalHandler::~SignalHandler() noexcept {
  // Restore the old signal handlers or actions.
  for (const auto& [signal_number, prev_action] : prev_actions_) {
    // NOLINTNEXTLINE(*-include-cleaner)
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

// Standard error stream file descriptor.
constexpr int STDERR_FD = 1;

// Dump a message in a signal handler friendly way.
template<class... Args>
void dump(std::format_string<Args...> fmt, Args&&... args) noexcept {
  try {
    std::array<char, 1024> buffer{};
    const auto result = std::format_to_n( //
        buffer.begin(), buffer.size(), fmt, std::forward<Args>(args)...);
    write(STDERR_FD, buffer.data(), result.size);
  } catch (...) {} // NOLINT(*-empty-catch)
}

// Dump backtrace in a signal handler friendly way.
void dump_backtrace() noexcept {
  constexpr int max_stack_depth = 64;
  std::array<void*, max_stack_depth> stack_trace{};
  const auto stack_depth = backtrace(stack_trace.data(), max_stack_depth);
  backtrace_symbols_fd(stack_trace.data(), stack_depth, STDERR_FD);
}

} // namespace

void SignalHandler::on_signal(int signal_number) noexcept {
  if (signal_number == SIGINT) {
    // Exit normally.
    dump("\n\n{}.\n", "Interrupted by Ctrl+C");
    exit(0);
  } else {
    // Dump backtrace and exit fast with an error.
    dump("\n\n{}", "Terminated by signal ");
    // NOLINTBEGIN(*-include-cleaner)
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
      default:      dump("#{}", signal_number); break;
    }
    // NOLINTEND(*-include-cleaner)
    dump(".\n");
    dump_backtrace();
    fast_exit(1);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
