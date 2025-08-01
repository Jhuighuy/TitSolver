/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <csignal>
#include <initializer_list>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include <execinfo.h>
#include <unistd.h>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/print.hpp"
#include "tit/core/sys/signal.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_raise(int signal_number) {
  const auto status = std::raise(signal_number);
  TIT_ENSURE(status == 0, "Failed to raise the signal {}!", signal_number);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::vector<SignalHandler*> SignalHandler::handlers_{};

SignalHandler::SignalHandler(std::initializer_list<int> signal_numbers) {
  // Register the current handler object.
  handlers_.push_back(this);

  // Register the new signal actions (or handlers).
  prev_handlers_.reserve(signal_numbers.size());
  for (const auto signal_number : signal_numbers) {
    // Register the new action and store the previous one.
    const auto prev_handler = std::signal(signal_number, &handle_signal_);
    TIT_ENSURE(prev_handler != SIG_ERR,
               "Unable to register the action for signal {}!",
               signal_number);
    prev_handlers_.emplace_back(signal_number, prev_handler);
  }
}

SignalHandler::~SignalHandler() noexcept { // NOLINT(*-exception-escape)
  // Restore the old signal handlers or actions.
  for (const auto& [signal_number, prev_handler] : prev_handlers_) {
    if (std::signal(signal_number, prev_handler) == SIG_ERR) {
      err("Unable to restore the previous handler for signal {}!",
          signal_number);
    }
  }

  // Unregister the current handler object.
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Dump a message in the "async-signal-safe" way.
void dump(std::string_view message) noexcept {
  std::ignore = write(STDERR_FILENO, message.data(), message.size());
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
    : SignalHandler{SIGINT,
                    SIGILL,
                    SIGABRT,
                    SIGFPE,
                    SIGSEGV,
                    SIGTRAP, // NOLINT(*-include-cleaner)
                    SIGTERM} {}

[[noreturn]] void FatalSignalHandler::on_signal(int signal_number) noexcept {
  const par::GlobalLock lock{};
  if (signal_number == SIGINT) {
    // Exit normally.
    dump("\n\nInterrupted by Ctrl+C.\n");
    exit(ExitCode::success);
  } else {
    // Dump backtrace and fast-exit with an error.
    dump("\n\nTerminated by ");
    dump(translate<std::string_view>(signal_number)
             .option(SIGILL, "SIGILL (illegal instruction)")
             .option(SIGABRT, "SIGABRT (aborted)")
             .option(SIGFPE, "SIGFPE (floating-point exception)")
             .option(SIGSEGV, "SIGSEGV (segmentation fault)")
             .option(SIGTRAP, "SIGTRAP (trace/breakpoint trap)")
             .option(SIGTERM, "SIGTERM")
             .fallback("unknown signal"));
    dump(".\n\nStack trace:\n");
    dump_backtrace();
    fast_exit(ExitCode::failure);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
