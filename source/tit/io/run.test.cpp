/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/io/run.hpp"
#include "tit/testing/test.hpp"

namespace tit::io {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto read_text(const std::filesystem::path& path) -> std::string {
  std::ifstream stream{path};
  return {std::istreambuf_iterator<char>{stream},
          std::istreambuf_iterator<char>{}};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("io::RunWriter publishes immutable HDF5 frames") {
  const std::filesystem::path path{"test.tit-run"};
  RunWriter writer{path, RunMetadata{"test", 2}};

  {
    auto frame = writer.begin_frame(0, 0.0);
    frame.write("id", std::vector<std::uint64_t>{11, 12});
    frame.write("kind", std::vector<std::uint8_t>{0, 0});
    frame.write("r", std::vector{Vec{1.0, 2.0}, Vec{3.0, 4.0}});
    frame.write("rho", std::vector{1000.0, 1001.0});
    frame.commit();
  }

  CHECK(std::filesystem::is_regular_file(path / "manifest.json"));
  CHECK(std::filesystem::is_regular_file(path / "index.json"));
  CHECK(std::filesystem::is_regular_file(path / "run.xdmf"));
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
  const auto xdmf = read_text(path / "run.xdmf");
  CHECK(xdmf.contains("<Time Value=\"0\"/>"));
  CHECK(xdmf.contains("Dimensions=\"2 2\" NumberType=\"Float\" "
                      "Precision=\"8\" Format=\"HDF\">frames/"
                      "frame-00000000.h5:/fields/r"));
  CHECK(xdmf.contains("Name=\"id\" AttributeType=\"Scalar\""));

  {
    auto next = writer.begin_frame(10, 0.5);
    next.write("id", std::vector<std::uint64_t>{11, 12});
    next.write("kind", std::vector<std::uint8_t>{0, 0});
    next.write("r", std::vector{Vec{1.5, 2.0}, Vec{3.5, 4.0}});
    next.write("rho", std::vector{1000.5, 1001.5});
    next.commit();
  }
  reader.refresh();
  REQUIRE(reader.num_frames() == 2);
  CHECK(reader.frame(1).descriptor() == FrameDescriptor{10, 0.5});
  CHECK(read_text(path / "run.xdmf").contains("<Time Value=\"0.5\"/>"));

  {
    auto checkpoint = writer.begin_checkpoint(10, 0.5);
    checkpoint.write("id", std::vector<std::uint64_t>{11, 12});
    checkpoint.write("kind", std::vector<std::uint8_t>{0, 0});
    checkpoint.write("r", std::vector{Vec{1.5, 2.0}, Vec{3.5, 4.0}});
    checkpoint.write("v", std::vector{Vec{0.1, 0.2}, Vec{0.3, 0.4}});
    checkpoint.write("rho", std::vector{1000.5, 1001.5});
    checkpoint.write("m", std::vector{1.0, 1.0});
    checkpoint.commit();
  }
  reader.refresh();
  REQUIRE(reader.num_checkpoints() == 1);
  CHECK(reader.checkpoint(0).descriptor() == FrameDescriptor{10, 0.5});
  CHECK(reader.checkpoint(0).read<Vec<double, 2>>("v") ==
        std::vector{Vec{0.1, 0.2}, Vec{0.3, 0.4}});

  reader.copy_to("exported.tit-run");
  const RunReader exported{"exported.tit-run"};
  REQUIRE(exported.num_frames() == 2);
  REQUIRE(exported.num_checkpoints() == 1);
  CHECK(exported.metadata() == reader.metadata());
  CHECK(std::filesystem::is_regular_file("exported.tit-run/run.xdmf"));
  CHECK(exported.frame(1).read<double>("rho") == std::vector{1000.5, 1001.5});
  CHECK(exported.checkpoint(0).read<double>("m") == std::vector{1.0, 1.0});

  std::filesystem::remove(path / "run.xdmf");
  reader.regenerate_xdmf();
  CHECK(read_text(path / "run.xdmf").contains("step-10"));
}

TEST_CASE("io::RunWriter abandons unpublished partial frames") {
  const std::filesystem::path path{"abandoned.tit-run"};
  RunWriter writer{path, RunMetadata{"abandoned", 2}};

  {
    auto frame = writer.begin_frame(0, 0.0);
    frame.write("id", std::vector<std::uint64_t>{11});
    frame.write("kind", std::vector<std::uint8_t>{0});
    frame.write("r", std::vector{Vec{1.0, 2.0}});
    frame.commit();
  }

  RunReader reader{path};
  {
    auto frame = writer.begin_frame(1, 0.1);
    frame.write("id", std::vector<std::uint64_t>{11});
    frame.write("kind", std::vector<std::uint8_t>{0});
    frame.write("r", std::vector{Vec{1.1, 2.0}});
    CHECK(std::filesystem::is_regular_file(path /
                                           "frames/frame-00000001.h5.partial"));
    reader.refresh();
    CHECK(reader.num_frames() == 1);
  }

  CHECK_FALSE(
      std::filesystem::exists(path / "frames/frame-00000001.h5.partial"));
  CHECK_FALSE(std::filesystem::exists(path / "frames/frame-00000001.h5"));
  reader.refresh();
  CHECK(reader.num_frames() == 1);

  {
    auto frame = writer.begin_frame(1, 0.1);
    frame.write("id", std::vector<std::uint64_t>{11});
    frame.write("kind", std::vector<std::uint8_t>{0});
    frame.write("r", std::vector{Vec{1.1, 2.0}});
    frame.commit();
  }
  reader.refresh();
  CHECK(reader.num_frames() == 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::io
