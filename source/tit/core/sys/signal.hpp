/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <initializer_list>
#include <ranges>
#include <utility>
#include <vector>

#ifdef __APPLE__
#include <sys/signal.h>
#else
#include <signal.h> // NOLINT(*-deprecated-headers)
#endif

#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Raise a signal.
void checked_raise(int signal_number);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Scoped signal handler.
class SignalHandler : public VirtualBase {
public:

  /// Initialize handling for the specified signals.
  explicit SignalHandler(std::initializer_list<int> signal_numbers);

  /// Reset signal handling.
  ~SignalHandler() noexcept override;

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

  std::vector<std::pair<int, struct sigaction>> prev_actions_;

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
