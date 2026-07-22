/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <vector>

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
  CHECK(communicator.all_reduce_max(static_cast<double>(rank) + 1.0) ==
        static_cast<double>(size));

  const auto rank_value = static_cast<std::uint64_t>(rank);
  const auto size_value = static_cast<std::uint64_t>(size);
  const auto value = rank_value + 1;
  const auto expected_sum = size_value * (size_value + 1) / 2;
  CHECK(communicator.all_reduce_sum(value) == expected_sum);
  CHECK(communicator.all_reduce_min(value) == 1);
  CHECK(communicator.all_reduce_max(value) == size_value);
  CHECK(communicator.all_reduce_sum(
            std::vector<std::uint64_t>{value, 2 * value}) ==
        std::vector<std::uint64_t>{expected_sum, 2 * expected_sum});

  const auto expected_prefix = rank_value * (rank_value + 1) / 2;
  CHECK(communicator.exclusive_scan_sum(value) == expected_prefix);

  REQUIRE(size <= 255);
  std::vector<std::vector<std::byte>> send_buffers(size);
  for (std::size_t destination = 0; destination < size; ++destination) {
    send_buffers[destination] = {
        static_cast<std::byte>(rank),
        static_cast<std::byte>(destination),
    };
  }
  const auto receive_buffers = communicator.all_to_all_bytes(send_buffers);
  REQUIRE(receive_buffers.size() == size);
  for (std::size_t source = 0; source < size; ++source) {
    CHECK(receive_buffers[source] == std::vector{static_cast<std::byte>(source),
                                                 static_cast<std::byte>(rank)});
  }
  communicator.barrier();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::dist
