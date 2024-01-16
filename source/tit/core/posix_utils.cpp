/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <vector>

#include <signal.h> // NOLINT(*-deprecated-headers)
#ifdef __APPLE__
#include <sys/signal.h>
#endif

#include "tit/core/assert.hpp"
#include "tit/core/posix_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
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

} // namespace tit
