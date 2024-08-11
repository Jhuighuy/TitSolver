/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/sys.hpp"
#pragma once

#include <initializer_list>
#include <ranges>
#include <utility>
#include <vector>

namespace tit {

#ifdef __APPLE__
#include <sys/signal.h>
#else
#include <signal.h> // NOLINT(*-deprecated-headers)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Signal action entry.
using sigaction_t = struct sigaction;

/// Set signal action.
void safe_sigaction(int signal_number,
                    const sigaction_t* action,
                    sigaction_t* prev_action = nullptr) noexcept;

/// Raise a signal.
void safe_raise(int signal_number) noexcept;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Scoped signal handler.
class SignalHandler {
public:

  /// Initialize handling for the specified signals.
  explicit SignalHandler(std::initializer_list<int> signal_numbers);

  /// Signal handler is not move-constructible.
  SignalHandler(SignalHandler&&) = delete;

  /// Signal handler is not movable.
  auto operator=(SignalHandler&&) -> SignalHandler& = delete;

  /// Signal handler is not copy-constructible.
  SignalHandler(const SignalHandler&) = delete;

  /// Signal handler is not copyable.
  auto operator=(const SignalHandler&) -> SignalHandler& = delete;

  /// Reset signal handling.
  virtual ~SignalHandler() noexcept;

  /// A range of handled signals.
  constexpr auto signals() const noexcept {
    return prev_actions_ | std::views::keys;
  }

protected:

  /// Signal interception callback.
  ///
  /// @note The implementation must be "async-signal-safe".
  virtual void on_signal(int signal_number) noexcept = 0;

private:

  std::vector<std::pair<int, sigaction_t>> prev_actions_;

  static std::vector<SignalHandler*> handlers_;
  static void handle_signal_(int signal_number) noexcept;

}; // class SignalHandler

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Signal handler that catches fatal signals and exits the process.
class FatalSignalHandler final : public SignalHandler {
public:

  /// Initialize handling for the fatal signals.
  FatalSignalHandler();

protected:

  [[noreturn]]
  void on_signal(int signal_number) noexcept override;

}; // class FatalSignalHandler

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
