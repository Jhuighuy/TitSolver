/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <csignal>
#include <initializer_list>
#include <utility>

#include <doctest/doctest.h>

#include "tit/core/assert.hpp"
#include "tit/core/signals.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Simple signal handler that stores all the handled signals. */
class SignalTracker final : public SignalHandler {
public:

  /** Setup the signal tracker. */
  SignalTracker(std::initializer_list<int> signal_numbers)
      : SignalHandler(signal_numbers) {}

  /** Retrieve the last handled signal number. */
  [[nodiscard]] auto last() noexcept -> int {
    return std::exchange(last_signal_number_, {});
  }

protected:

  void on_signal(int signal_number) noexcept final {
    CHECK_MESSAGE(last_signal_number_ == 0, "Signal tracker was not reset!");
    last_signal_number_ = signal_number;
  }

private:

  int last_signal_number_ = 0;

}; // class SignalTracker

// Safety wrapper.
void safe_raise(int signal_number) noexcept {
  const auto status = raise(signal_number);
  TIT_ENSURE(status == 0, "Failed to raise a signal.");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::SignalHandler") {
  // NOLINTBEGIN(*-include-cleaner)
  SignalTracker handler_1{SIGUSR1, SIGUSR2};
  {
    SignalTracker handler_2{SIGUSR2};
    // Raise the signal that shall be handled by the first tracker.
    safe_raise(SIGUSR1);
    // Raise the signal that it shall be handled by the second tracker.
    safe_raise(SIGUSR2);
    // Check what was handled.
    CHECK(handler_1.last() == SIGUSR1);
    CHECK(handler_2.last() == SIGUSR2);
  }
  // Raise the signal that shall be handled by the first tracker.
  safe_raise(SIGUSR2);
  // Raise the signal that shall not be handled by any trackers.
  safe_raise(SIGCHLD);
  // Check what was handled.
  CHECK(handler_1.last() == SIGUSR2);
  // NOLINTEND(*-include-cleaner)
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
