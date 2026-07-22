/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/io/parallel_run.hpp"
#include "tit/io/run.hpp"
#include "tit/testing/test.hpp"

namespace tit::io {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[maybe_unused]] const dist::Environment environment{};

TEST_CASE("io::ParallelRunWriter writes collective rank-major hyperslabs") {
  const auto communicator = dist::Communicator::world();
  REQUIRE(communicator.size() == 2);
  ParallelRunWriter writer{"parallel.tit-run",
                           RunMetadata{"parallel-test", 2},
                           communicator};

  {
    const auto ids = communicator.rank() == 0 ?
                         std::vector<std::uint64_t>{1, 2} :
                         std::vector<std::uint64_t>{3};
    const auto positions = communicator.rank() == 0 ?
                               std::vector{Vec{1.0, 0.0}, Vec{2.0, 0.0}} :
                               std::vector{Vec{3.0, 0.0}};
    const auto density = communicator.rank() == 0 ?
                             std::vector{1001.0, 1002.0} :
                             std::vector{1003.0};
    const std::vector<std::uint8_t> kinds(ids.size(), 0);

    auto frame = writer.begin_frame(0, 0.0);
    frame.write("id", ids);
    frame.write("kind", kinds);
    frame.write("r", positions);
    frame.write("rho", density);
    frame.commit();
  }

  {
    const auto ids = communicator.rank() == 0 ?
                         std::vector<std::uint64_t>{} :
                         std::vector<std::uint64_t>{4, 5};
    const auto positions = communicator.rank() == 0 ?
                               std::vector<Vec<double, 2>>{} :
                               std::vector{Vec{4.0, 0.0}, Vec{5.0, 0.0}};
    const auto density = communicator.rank() == 0 ? std::vector<double>{} :
                                                    std::vector{1004.0, 1005.0};
    const std::vector<std::uint8_t> kinds(ids.size(), 0);

    auto frame = writer.begin_frame(10, 1.0);
    frame.write("id", ids);
    frame.write("kind", kinds);
    frame.write("r", positions);
    frame.write("rho", density);
    frame.commit();
  }

  if (communicator.rank() == 0) {
    const RunReader reader{"parallel.tit-run"};
    REQUIRE(reader.num_frames() == 2);
    CHECK(reader.frame(0).read<std::uint64_t>("id") ==
          std::vector<std::uint64_t>{1, 2, 3});
    CHECK(reader.frame(0).read<Vec<double, 2>>("r") ==
          std::vector{Vec{1.0, 0.0}, Vec{2.0, 0.0}, Vec{3.0, 0.0}});
    CHECK(reader.frame(1).read<std::uint64_t>("id") ==
          std::vector<std::uint64_t>{4, 5});
    CHECK(reader.frame(1).read<double>("rho") == std::vector{1004.0, 1005.0});
  }
  communicator.barrier();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::io
