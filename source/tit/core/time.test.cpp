/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <thread>

#include "tit/core/basic_types.hpp"
#include "tit/core/time.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Stopwatch") {
  // Run the stopwatch to measure some time.
  Stopwatch stopwatch{};
  const auto delta = std::chrono::milliseconds(500);
  const auto delta_sec = 1.0e-3 * static_cast<float64_t>(delta.count());
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
