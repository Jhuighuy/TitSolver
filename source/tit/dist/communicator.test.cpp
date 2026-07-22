/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>

#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/testing/test.hpp"

namespace tit::dist {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[maybe_unused]] const Environment environment{};

TEST_CASE("dist::Communicator collectives") {
  const auto communicator = Communicator::world();
  const auto rank = communicator.rank();
  const auto size = communicator.size();

  REQUIRE(rank < size);
  CHECK(communicator.all_reduce_min(static_cast<double>(rank) + 1.0) == 1.0);

  const auto rank_value = static_cast<std::uint64_t>(rank);
  const auto size_value = static_cast<std::uint64_t>(size);
  const auto value = rank_value + 1;
  const auto expected_sum = size_value * (size_value + 1) / 2;
  CHECK(communicator.all_reduce_sum(value) == expected_sum);

  const auto expected_prefix = rank_value * (rank_value + 1) / 2;
  CHECK(communicator.exclusive_scan_sum(value) == expected_prefix);
  communicator.barrier();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::dist
