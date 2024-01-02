/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <csignal> // IWYU pragma: keep
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <vector>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** POSIX signal handler.
\******************************************************************************/
class SignalHandler {
public:

  /** Initialize signal handling for the common signals. */
  SignalHandler();
  /** Initialize signal handling for the specified signals. */
  SignalHandler(std::initializer_list<int> signal_numbers);

  /** Signal handler is not move-constructible. */
  SignalHandler(SignalHandler&&) = delete;
  /** Signal handler is not movable. */
  auto operator=(SignalHandler&&) -> SignalHandler& = delete;

  /** Signal handler is not copy-constructible. */
  SignalHandler(const SignalHandler&) = delete;
  /** Signal handler is not copyable. */
  auto operator=(const SignalHandler&) -> SignalHandler& = delete;

  /** Reset signal handling. */
  virtual ~SignalHandler() noexcept;

  /** A range of handled signals. */
  auto signals() const noexcept {
    return prev_actions_ | std::views::keys;
  }

protected:

  /** Signal interception callback.
   ** @note The implementation must be "async-signal-safe". */
  virtual void on_signal(int signal_number) noexcept;

private:

  using sigaction_t = struct sigaction; // NOLINT(*-include-cleaner)
  std::vector<std::tuple<int, sigaction_t>> prev_actions_;

  // NOLINTNEXTLINE(*-avoid-non-const-global-variables)
  static std::vector<SignalHandler*> handlers_;
  static void handle_signal_(int signal_number) noexcept;

}; // class SignalHandler

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
