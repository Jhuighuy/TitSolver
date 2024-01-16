/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <initializer_list>
#include <ranges>
#include <tuple>
#include <vector>

#ifdef __APPLE__
#include <sys/signal.h>
#else
#include <signal.h> // NOLINT(*-deprecated-headers)
#endif

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** POSIX signal handler.
\******************************************************************************/
class SignalHandler {
public:

  /** Initialize handling for the specified signals. */
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
  constexpr auto signals() const noexcept {
    return prev_actions_ | std::views::keys;
  }

protected:

  /** Signal interception callback.
   ** @note The implementation must be "async-signal-safe". */
  virtual void on_signal(int signal_number) noexcept = 0;

private:

  using sigaction_t = struct sigaction;
  std::vector<std::tuple<int, sigaction_t>> prev_actions_;

  // NOLINTNEXTLINE(*-avoid-non-const-global-variables)
  static std::vector<SignalHandler*> handlers_;
  static void handle_signal_(int signal_number) noexcept;

}; // class SignalHandler

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
