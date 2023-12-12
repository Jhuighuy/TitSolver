/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <csignal>
#include <functional>
#include <tuple>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"
#include "tit/core/posix.hpp"

namespace tit::posix {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::vector<const SignalHandler*> SignalHandler::handlers_{};

#if !TIT_IWYU
SignalHandler::SignalHandler()
    : SignalHandler({SIGINT, SIGTERM, SIGABRT, SIGSEGV, SIGILL, SIGFPE}) {}
#endif

SignalHandler::SignalHandler(std::initializer_list<int> signal_numbers) {
  // Register the current handler object.
  handlers_.push_back(this);
  // Register the new signal actions (or handlers).
#if TIT_HAVE_SIGACTION
  prev_actions_.reserve(signal_numbers.size());
  for (const auto signal_number : signal_numbers) {
    // Prepare the new action.
    sigaction_t signal_action{};
    signal_action.sa_flags = 0;
    signal_action.sa_handler = &handle_signal_;
    sigemptyset(&signal_action.sa_mask);
    // Register the new action and store the previous one.
    sigaction_t prev_signal_action{};
    const auto status =
        sigaction(signal_number, &signal_action, &prev_signal_action);
    TIT_ENSURE(status == 0, "Unable to set the signal action!");
    prev_actions_.emplace_back(signal_number, prev_signal_action);
  }
#else
  prev_handlers_.reserve(signal_numbers.size());
  for (const auto signal_number : signal_numbers) {
    // Register the new handler and store the previous one.
    const auto prev_signal_handler = signal(signal_number, &handle_signal_);
    TIT_ENSURE(prev_signal_handler != SIG_ERR,
               "Unable to set the signal handler!");
    prev_handlers_.emplace_back(signal_number, prev_signal_handler);
  }
#endif
}

SignalHandler::~SignalHandler() noexcept {
  // Restore the old signal handlers or actions.
#if TIT_HAVE_SIGACTION
  for (auto& [signal_number, prev_signal_action] : prev_actions_) {
    const auto status = sigaction(signal_number, &prev_signal_action, nullptr);
    TIT_ENSURE(status == 0, "Unable to reset the signal action!");
  }
#else
  for (const auto& [signal_number, prev_signal_handler] : prev_handlers_) {
    const auto signal_handler = signal(signal_number, prev_signal_handler);
    TIT_ENSURE(signal_handler != SIG_ERR,
               "Unable to reset the signal handler!");
  }
#endif
  // Unregister the current signal handler.
  TIT_ASSERT(handlers_.back() == this, "Signal handler was not registered!");
  handlers_.pop_back();
}

void SignalHandler::handle_signal_(int signal_number) noexcept {
  // Traverse the registered handler and find the one that handles the signal
  // that we've intercepted.
  for (const auto* handler : handlers_ | std::views::reverse) {
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

} // namespace tit::posix
