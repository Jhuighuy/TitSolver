/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>

#include "tit/core/proc_info.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("compute_cpu_percent") {
  const proc_info::UsageSnapshot previous{
      .cpu_time_ns = 100'000'000,
      .memory_bytes = 64,
  };

  SUBCASE("single saturated core") {
    const proc_info::UsageSnapshot next{
        .cpu_time_ns = 1'100'000'000,
        .memory_bytes = 64,
    };
    CHECK(proc_info::compute_cpu_percent(previous,
                                         next,
                                         std::chrono::seconds{1}) ==
          doctest::Approx{100.0});
  }

  SUBCASE("multiple saturated cores") {
    const proc_info::UsageSnapshot next{
        .cpu_time_ns = 8'100'000'000,
        .memory_bytes = 64,
    };
    CHECK(proc_info::compute_cpu_percent(previous,
                                         next,
                                         std::chrono::seconds{1}) ==
          doctest::Approx{800.0});
  }

  SUBCASE("non-positive wall delta") {
    const proc_info::UsageSnapshot next{
        .cpu_time_ns = 1'100'000'000,
        .memory_bytes = 64,
    };
    CHECK(proc_info::compute_cpu_percent(previous,
                                         next,
                                         std::chrono::nanoseconds::zero()) ==
          doctest::Approx{0.0});
  }

  SUBCASE("backwards cpu time") {
    const proc_info::UsageSnapshot next{
        .cpu_time_ns = 50'000'000,
        .memory_bytes = 64,
    };
    CHECK(proc_info::compute_cpu_percent(previous,
                                         next,
                                         std::chrono::seconds{1}) ==
          doctest::Approx{0.0});
  }
}

TEST_CASE("usage snapshot stores memory bytes") {
  const proc_info::UsageSnapshot snapshot{
      .cpu_time_ns = 42,
      .memory_bytes = 1024ULL * 1024ULL * 512ULL,
  };

  CHECK(snapshot.memory_bytes == 1024ULL * 1024ULL * 512ULL);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
