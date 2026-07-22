/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <filesystem>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/io/run.hpp"
#include "tit/testing/test.hpp"

namespace tit::io {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("io::RunWriter publishes immutable HDF5 frames") {
  const std::filesystem::path path{"test.tit-run"};
  RunWriter writer{path, RunMetadata{"test", 2}};

  {
    auto frame = writer.begin_frame(0, 0.0);
    frame.write("id", std::vector<std::uint64_t>{11, 12});
    frame.write("r", std::vector{Vec{1.0, 2.0}, Vec{3.0, 4.0}});
    frame.write("rho", std::vector{1000.0, 1001.0});
    frame.commit();
  }

  CHECK(std::filesystem::is_regular_file(path / "manifest.json"));
  CHECK(std::filesystem::is_regular_file(path / "index.json"));
  CHECK(std::filesystem::is_regular_file(path / "frames/frame-00000000.h5"));
  CHECK_FALSE(
      std::filesystem::exists(path / "frames/frame-00000000.h5.partial"));

  RunReader reader{path};
  CHECK(reader.metadata() == RunMetadata{"test", 2});
  REQUIRE(reader.num_frames() == 1);
  const auto frame = reader.frame(0);
  CHECK(frame.descriptor() == FrameDescriptor{0, 0.0});
  CHECK(frame.read<std::uint64_t>("id") == std::vector<std::uint64_t>{11, 12});
  CHECK(frame.read<Vec<double, 2>>("r") ==
        std::vector{Vec{1.0, 2.0}, Vec{3.0, 4.0}});
  CHECK(frame.read<double>("rho") == std::vector{1000.0, 1001.0});

  {
    auto next = writer.begin_frame(10, 0.5);
    next.write("id", std::vector<std::uint64_t>{11, 12});
    next.write("r", std::vector{Vec{1.5, 2.0}, Vec{3.5, 4.0}});
    next.write("rho", std::vector{1000.5, 1001.5});
    next.commit();
  }
  reader.refresh();
  REQUIRE(reader.num_frames() == 2);
  CHECK(reader.frame(1).descriptor() == FrameDescriptor{10, 0.5});

  reader.copy_to("exported.tit-run");
  const RunReader exported{"exported.tit-run"};
  REQUIRE(exported.num_frames() == 2);
  CHECK(exported.metadata() == reader.metadata());
  CHECK(exported.frame(1).read<double>("rho") == std::vector{1000.5, 1001.5});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::io
