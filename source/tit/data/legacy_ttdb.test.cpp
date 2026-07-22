/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/data/legacy_ttdb.hpp"
#include "tit/data/storage.hpp"
#include "tit/io/run.hpp"
#include "tit/testing/test.hpp"

namespace tit::data {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::convert_ttdb creates a logical HDF5 run") {
  const std::filesystem::path source{"legacy.ttdb"};
  const std::filesystem::path destination{"converted.tit-run"};
  {
    Storage storage{source};
    const auto series = storage.create_series("legacy-test");
    for (std::size_t index = 0; index < 2; ++index) {
      const auto value = static_cast<double>(index);
      const auto frame = series.create_frame(value);
      frame.create_array("r").write(
          std::vector{Vec{1.0 + value, 2.0}, Vec{3.0 + value, 4.0}});
      frame.create_array("v").write(
          std::vector{Vec{0.1 + value, 0.2}, Vec{0.3 + value, 0.4}});
      frame.create_array("rho").write(
          std::vector{1000.0 + value, 1001.0 + value});
    }
  }

  convert_ttdb(source, destination);

  const io::RunReader run{destination};
  CHECK(run.metadata() == io::RunMetadata{"legacy-test", 2});
  REQUIRE(run.num_frames() == 2);
  CHECK(run.num_checkpoints() == 0);
  CHECK(run.frame(0).read<std::uint64_t>("id") ==
        std::vector<std::uint64_t>{0, 1});
  CHECK(run.frame(0).read<std::uint8_t>("kind") ==
        std::vector<std::uint8_t>{0, 0});
  CHECK(run.frame(1).read<Vec<double, 2>>("r") ==
        std::vector{Vec{2.0, 2.0}, Vec{4.0, 4.0}});
  CHECK(run.frame(1).read<double>("rho") == std::vector{1001.0, 1002.0});
  CHECK(std::filesystem::is_regular_file(destination / "run.xdmf"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::data
