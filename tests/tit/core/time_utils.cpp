/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include <doctest/doctest.h>

#include "tit/core/time_utils.hpp"
#include "tit/core/types.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::Stopwatch") {
  // Run the stopwatch to measure some time.
  Stopwatch stopwatch{};
  const auto delta = std::chrono::milliseconds(500);
  const auto delta_sec = 1.0e-3 * static_cast<real_t>(delta.count());
  {
    const StopwatchCycle cycle{stopwatch};
    std::this_thread::sleep_for(delta);
  }
  // Ensure the measured time is correct. Unfortunately, we cannot check for
  // accuracy due to the process scheduling on CI (and hence timing) is very
  // unstable.
  CHECK(stopwatch.total() >= delta_sec);
  CHECK(stopwatch.cycle() >= delta_sec);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
