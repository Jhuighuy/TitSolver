/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <csignal>
#include <initializer_list>
#include <utility>

#include "tit/core/sys/signal.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Simple signal handler that stores all the handled signals.
class SignalTracker final : public SignalHandler {
public:

  // Setup the signal tracker.
  SignalTracker(std::initializer_list<int> signal_numbers)
      : SignalHandler(signal_numbers) {}

  // Retrieve the last handled signal number.
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("SignalHandler") {
  SignalTracker handler_1{SIGABRT, SIGTERM};
  {
    SignalTracker handler_2{SIGTERM};

    // Raise the signal that shall be handled by the first tracker.
    checked_raise(SIGABRT);

    // Raise the signal that it shall be handled by the second tracker.
    checked_raise(SIGTERM);

    // Check what was handled.
    CHECK(handler_1.last() == SIGABRT);
    CHECK(handler_2.last() == SIGTERM);
  }

  // Raise the signal that shall be handled by the first tracker.
  checked_raise(SIGTERM);

  // Check what was handled.
  CHECK(handler_1.last() == SIGTERM);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
