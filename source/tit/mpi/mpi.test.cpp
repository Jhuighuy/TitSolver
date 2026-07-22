/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

#include "tit/mpi/mpi.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// The tests below are written to hold for any number of processes, and are
// exercised at one, two and four processes by the test registrations.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::rank") {
  REQUIRE(mpi::initialized());
  const auto rank = mpi::world.rank();
  const auto size = mpi::world.size();
  CHECK(size >= 1);
  CHECK(rank < size);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::all_reduce") {
  const auto rank = mpi::world.rank();
  const auto size = mpi::world.size();

  SUBCASE("scalar") {
    // Reduce the process ranks.
    const auto value = static_cast<std::int64_t>(rank) + 1;
    CHECK(mpi::world.all_reduce(value, mpi::Op::min) == 1);
    CHECK(mpi::world.all_reduce(value, mpi::Op::max) ==
          static_cast<std::int64_t>(size));
    CHECK(mpi::world.all_reduce(value, mpi::Op::sum) ==
          static_cast<std::int64_t>(size * (size + 1) / 2));
  }

  SUBCASE("in-place") {
    // Element-wise reduction of small per-process vectors.
    std::vector<double> values{static_cast<double>(rank), 10.0};
    mpi::world.all_reduce(std::span{values}, mpi::Op::max);
    CHECK(values[0] == static_cast<double>(size - 1));
    CHECK(values[1] == 10.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::all_gather") {
  const auto rank = mpi::world.rank();
  const auto size = mpi::world.size();

  // Gather the (transformed) ranks from all the processes.
  const auto gathered = mpi::world.all_gather(3 * rank + 7);
  REQUIRE(gathered.size() == size);
  for (const auto [index, value] : std::views::enumerate(gathered)) {
    CHECK(value == 3 * static_cast<std::size_t>(index) + 7);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::all_to_all") {
  const auto rank = mpi::world.rank();
  const auto size = mpi::world.size();

  SUBCASE("counts") {
    // Each process sends `10 * my_rank + dest_rank` to each destination.
    std::vector<std::size_t> send_counts(size);
    for (const auto dest : std::views::iota(std::size_t{0}, size)) {
      send_counts[dest] = 10 * rank + dest;
    }
    const auto recv_counts = mpi::world.all_to_all(send_counts);
    REQUIRE(recv_counts.size() == size);
    for (const auto source : std::views::iota(std::size_t{0}, size)) {
      CHECK(recv_counts[source] == 10 * source + rank);
    }
  }

  SUBCASE("bytes") {
    // Each process sends `dest + 1` copies of the byte `my_rank` to each
    // destination process.
    std::vector<std::size_t> send_counts(size);
    for (const auto dest : std::views::iota(std::size_t{0}, size)) {
      send_counts[dest] = dest + 1;
    }
    std::vector<std::byte> send_bytes{};
    for (const auto dest : std::views::iota(std::size_t{0}, size)) {
      send_bytes.resize(send_bytes.size() + send_counts[dest],
                        static_cast<std::byte>(rank));
    }

    // Every process, therefore, receives `my_rank + 1` bytes from each
    // source process, valued by the source rank.
    const auto recv_counts = mpi::world.all_to_all(send_counts);
    std::vector<std::size_t> expected_recv_counts(size, rank + 1);
    CHECK(recv_counts == expected_recv_counts);
    std::vector<std::byte> recv_bytes((rank + 1) * size);
    mpi::world.all_to_all_v(send_bytes, send_counts, recv_bytes, recv_counts);
    for (const auto source : std::views::iota(std::size_t{0}, size)) {
      for (const auto index : std::views::iota(std::size_t{0}, rank + 1)) {
        CHECK(recv_bytes[source * (rank + 1) + index] ==
              static_cast<std::byte>(source));
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::send") {
  const auto rank = mpi::world.rank();
  const auto size = mpi::world.size();
  if (size < 2) return; // Nothing to exchange in a single-process run.

  // Pass a value around the ring: each process sends to the next rank and
  // receives from the previous one.
  const auto next = (rank + 1) % size;
  const auto prev = (rank + size - 1) % size;
  const auto sent = std::bit_cast<std::array<std::byte, 8>>(
      static_cast<std::uint64_t>(100 + rank));
  std::array<std::byte, 8> received{};
  if (rank % 2 == 0) {
    mpi::world.send(sent, next);
    mpi::world.recv(received, prev);
  } else {
    mpi::world.recv(received, prev);
    mpi::world.send(sent, next);
  }
  CHECK(std::bit_cast<std::uint64_t>(received) == 100 + prev);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("mpi::Comm::barrier") {
  mpi::world.barrier();
  CHECK(true);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
