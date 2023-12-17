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

#include "tit/core/config.hpp"

namespace tit::posix {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** POSIX signal handler.
\******************************************************************************/
class SignalHandler {
public:

  /** Initialize signal handling for the common signals. */
  SignalHandler()
#if TIT_IWYU
      = default
#endif
      ;
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
#if TIT_HAVE_SIGACTION
    return prev_actions_ | std::views::keys;
#else
    return prev_handlers_ | std::views::keys;
#endif
  }

protected:

  /** Signal interception callback. */
  virtual void on_signal(int signal_number) const = 0;

private:

#if TIT_HAVE_SIGACTION
  using sigaction_t = struct sigaction;
  std::vector<std::tuple<int, sigaction_t>> prev_actions_;
#else
  using sighandler_t = void (*)(int);
  std::vector<std::tuple<int, sighandler_t>> prev_handlers_;
#endif

  static std::vector<const SignalHandler*> handlers_;
  static void handle_signal_(int signal_number) noexcept;

}; // class SignalHandler

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::posix
