/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <csignal>
#include <initializer_list>

#include <doctest/doctest.h>

#include "tit/core/posix.hpp"

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class MyHandler final : public tit::posix::SignalHandler {
public:

  MyHandler(int& handled_signal_number,
            std::initializer_list<int> signal_numbers)
      : tit::posix::SignalHandler(signal_numbers),
        handled_signal_number_{&handled_signal_number} {}

protected:

  void on_signal(int signal_number) const final {
    *handled_signal_number_ = signal_number;
  }

private:

  int* handled_signal_number_;

}; // class MyHandler

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTBEGIN(cert-err33-c)

TEST_CASE("tit::core::posix::SignalHandler") {
  int handled_1 = 0, handled_2 = 0;
  const auto reset = [&] { handled_1 = handled_2 = 0; };
  { // Create the first handler for two signals.
    const MyHandler handler_1(handled_1, {SIGUSR1, SIGUSR2});
    {
      // Create the second handler for the two signals.
      const MyHandler handler_2(handled_2, {SIGUSR2, SIGCHLD});
      {
        // Raise the signal that shall be intercepted by the first handler.
        reset();
        raise(SIGUSR1);
        CHECK(handled_1 == SIGUSR1);
        CHECK(handled_2 == 0);
        // Raise the signal that it shall be intercepted by the second handler.
        reset();
        raise(SIGUSR2);
        CHECK(handled_1 == 0);
        CHECK(handled_2 == SIGUSR2);
      }
    }
    // Raise the signal that shall be intercepted by the first handler.
    reset();
    raise(SIGUSR2);
    CHECK(handled_1 == SIGUSR2);
    CHECK(handled_2 == 0);
    // Raise the signal that shall not be intercepted by any handlers.
    reset();
    raise(SIGCHLD);
    CHECK(handled_1 == 0);
    CHECK(handled_2 == 0);
  }
}

// NOLINTEND(cert-err33-c)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
