/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <chrono> // IWYU pragma: keep
#include <thread>

#include <doctest/doctest.h>

#include "tit/core/math_utils.hpp"
#include "tit/core/time_utils.hpp"
#include "tit/core/types.hpp"

namespace tit {
namespace {

using namespace std::chrono_literals;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::Stopwatch") {
  // Run the stopwatch to measure some time.
  Stopwatch stopwatch{};
  constexpr std::array deltas{30ms, 100ms, 500ms}; // NOLINT(*-include-cleaner)
  std::chrono::milliseconds total{};
  for (const auto delta : deltas) {
    total += delta;
    const StopwatchCycle cycle{stopwatch};
    std::this_thread::sleep_for(delta);
  }
  // Ensure the amount of cycles is correct.
  CHECK(stopwatch.cycles() == deltas.size());
  // Ensure the measured time is correct (5% relative error).
  const auto total_sec = static_cast<real_t>(total.count()) * 1.0e-3;
  const auto cycle_sec = total_sec / deltas.size();
  CHECK(abs(stopwatch.total() - total_sec) < 0.05 * total_sec);
  CHECK(abs(stopwatch.cycle() - cycle_sec) < 0.05 * cycle_sec);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
