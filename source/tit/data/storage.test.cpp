/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <numbers>
#include <set>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/serialization.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::Storage") {
  const std::filesystem::path file_name{"test.ttdb"};
  const std::filesystem::path invalid_file_name{"/invalid/path/to/file.ttdb"};
  SUBCASE("success") {
    SUBCASE("create in-memory") {
      const data::Storage storage{":memory:"};
      CHECK(storage.path().empty());
    }
    SUBCASE("create file") {
      if (std::filesystem::exists(file_name)) {
        // Remove the file if it already exists.
        REQUIRE(std::filesystem::remove(file_name));
      }
      const data::Storage storage{file_name};
      CHECK(std::filesystem::exists(file_name));
      CHECK(storage.path().filename() == file_name);
    }
    SUBCASE("open existing") {
      // Should exist due to the previous test.
      REQUIRE(std::filesystem::exists(file_name));
      const data::Storage storage{file_name};
      CHECK(storage.path().filename() == file_name);
    }
    SUBCASE("open readonly") {
      // Should exist due to the previous test.
      REQUIRE(std::filesystem::exists(file_name));
      data::Storage storage{file_name, /*read_only=*/true};
      CHECK_THROWS_MSG(storage.create_series_id("test"),
                       Exception,
                       "attempt to write a readonly database");
    }
  }
  SUBCASE("failure") {
    SUBCASE("cannot create") {
      REQUIRE(!std::filesystem::exists(invalid_file_name));
      CHECK_THROWS_MSG(data::sqlite::Database{invalid_file_name},
                       Exception,
                       "unable to open database file");
    }
    SUBCASE("invalid file") {
      // Our current source file definitely is not a valid `.ttdb` file.
      CHECK_THROWS_MSG(data::sqlite::Database{__FILE__},
                       Exception,
                       "file is not a database");
    }
    /// @todo We shall add tests for valid database files with invalid schema.
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::SeriesView") {
  SUBCASE("empty storage") {
    const data::Storage storage{":memory:"};
    CHECK(storage.num_series() == 0);
    // CHECK(std::ranges::empty(storage.series_ids()));
  }
  SUBCASE("create series") {
    data::Storage storage{":memory:"};
    REQUIRE(storage.max_series() >= 3);

    const auto series_1 = storage.create_series("1");
    REQUIRE(storage.check_series(series_1));
    CHECK(series_1 == data::SeriesID{1});
    CHECK(series_1.name() == "1");
    CHECK(storage.num_series() == 1);
    CHECK_RANGE_EQ(storage.series(), {series_1});
    CHECK(storage.last_series() == series_1);

    const auto series_2 = storage.create_series("2");
    REQUIRE(storage.check_series(series_2));
    CHECK(series_2 == data::SeriesID{2});
    CHECK(series_2.name() == "2");
    CHECK(storage.num_series() == 2);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_2});
    CHECK(storage.last_series() == series_2);

    const auto series_3 = storage.create_series("3");
    REQUIRE(storage.check_series(series_3));
    CHECK(series_3 == data::SeriesID{3});
    CHECK(series_3.name() == "3");
    CHECK(storage.num_series() == 3);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_2, series_3});
    CHECK(storage.last_series() == series_3);
  }
  SUBCASE("create more series than the maximum") {
    data::Storage storage{":memory:"};
    storage.set_max_series(3);
    REQUIRE(storage.max_series() == 3);

    // Create a few series.
    const auto series_1 = storage.create_series("1");
    const auto series_2 = storage.create_series("2");
    const auto series_3 = storage.create_series("3");
    REQUIRE_RANGE_EQ(storage.series(), {series_1, series_2, series_3});

    // Create more series than the maximum. The oldest series should be removed.
    const auto series_4 = storage.create_series("4");
    CHECK(storage.check_series(series_4));
    CHECK_FALSE(storage.check_series(series_1));
    CHECK_RANGE_EQ(storage.series(), {series_2, series_3, series_4});

    const auto series_5 = storage.create_series("5");
    CHECK(storage.check_series(series_5));
    CHECK_FALSE(storage.check_series(series_2));
    CHECK_RANGE_EQ(storage.series(), {series_3, series_4, series_5});
  }
  SUBCASE("decrease maximum") {
    data::Storage storage{":memory:"};
    storage.set_max_series(3);
    REQUIRE(storage.max_series() == 3);

    // Create a few series.
    const auto series_1 = storage.create_series("1");
    const auto series_2 = storage.create_series("2");
    const auto series_3 = storage.create_series("3");
    REQUIRE_RANGE_EQ(storage.series(), {series_1, series_2, series_3});

    // Decrease the maximum. The oldest series should be removed.
    storage.set_max_series(2);
    CHECK(storage.max_series() == 2);
    CHECK_RANGE_EQ(storage.series(), {series_2, series_3});
  }
  SUBCASE("increase maximum") {
    data::Storage storage{":memory:"};
    storage.set_max_series(3);
    REQUIRE(storage.max_series() == 3);

    // Create a few series.
    const auto series_1 = storage.create_series("1");
    const auto series_2 = storage.create_series("2");
    const auto series_3 = storage.create_series("3");
    REQUIRE_RANGE_EQ(storage.series(), {series_1, series_2, series_3});

    // Increase the maximum.
    storage.set_max_series(5);
    CHECK(storage.max_series() == 5);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_2, series_3});

    // Create more series. Maximum should not be exceeded.
    const auto series_4 = storage.create_series("4");
    CHECK(storage.check_series(series_4));
    const auto series_5 = storage.create_series("5");
    CHECK(storage.check_series(series_5));
    CHECK_RANGE_EQ(storage.series(),
                   {series_1, series_2, series_3, series_4, series_5});
  }
  SUBCASE("delete series") {
    data::Storage storage{":memory:"};
    storage.set_max_series(3);
    REQUIRE(storage.max_series() == 3);

    // Create a few series.
    const auto series_1 = storage.create_series("1");
    const auto series_2 = storage.create_series("2");
    const auto series_3 = storage.create_series("3");
    REQUIRE_RANGE_EQ(storage.series(), {series_1, series_2, series_3});

    // Delete the series.
    storage.delete_series(series_2);
    CHECK_FALSE(storage.check_series(series_2));
    CHECK_RANGE_EQ(storage.series(), {series_1, series_3});

    // Create more series. Make sure the ID of the removed series is not reused.
    const auto series_4 = storage.create_series("4");
    CHECK(storage.check_series(series_4));
    CHECK(series_4 != series_2);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_3, series_4});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::FrameView") {
  SUBCASE("empty series") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    CHECK(series.num_frames() == 0);
    // CHECK(std::ranges::empty(series.frames()));
  }
  SUBCASE("create frames") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");

    const auto frame_1 = series.create_frame(0.0);
    REQUIRE(storage.check_frame(frame_1));
    CHECK(frame_1 == data::FrameID{1});
    CHECK(frame_1.time() == 0.0);
    CHECK(series.num_frames() == 1);
    CHECK_RANGE_EQ(series.frames(), {frame_1});
    CHECK(series.last_frame() == frame_1);

    const auto frame_2 = series.create_frame(1.0);
    REQUIRE(storage.check_frame(frame_2));
    CHECK(frame_2 == data::FrameID{2});
    CHECK(frame_2.time() == 1.0);
    CHECK(series.num_frames() == 2);
    CHECK_RANGE_EQ(series.frames(), {frame_1, frame_2});
    CHECK(series.last_frame() == frame_2);

    const auto frame_3 = series.create_frame(2.0);
    REQUIRE(storage.check_frame(frame_3));
    CHECK(frame_3 == data::FrameID{3});
    CHECK(frame_3.time() == 2.0);
    CHECK(series.num_frames() == 3);
    CHECK_RANGE_EQ(series.frames(), {frame_1, frame_2, frame_3});
    CHECK(series.last_frame() == frame_3);
  }
  SUBCASE("create frames in different series") {
    data::Storage storage{":memory:"};
    const auto series_1 = storage.create_series("");
    const auto series_2 = storage.create_series("");

    // Create frames in the first series.
    const auto frame_11 = series_1.create_frame(0.0);
    const auto frame_12 = series_1.create_frame(1.0);
    const auto frame_13 = series_1.create_frame(2.0);
    REQUIRE_RANGE_EQ(series_1.frames(), {frame_11, frame_12, frame_13});

    // Create frames in the second series.
    const auto frame_21 = series_2.create_frame(0.0);
    const auto frame_22 = series_2.create_frame(1.0);
    const auto frame_23 = series_2.create_frame(2.0);
    REQUIRE_RANGE_EQ(series_2.frames(), {frame_21, frame_22, frame_23});

    // Make sure the frames are not shared between series.
    const std::set<data::FrameID> all_frames{
        frame_11,
        frame_12,
        frame_13,
        frame_21,
        frame_22,
        frame_23,
    };
    CHECK(all_frames.size() == 6);
  }
  SUBCASE("delete frames") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");

    // Create a few frames.
    const auto frame_1 = series.create_frame(0.0);
    const auto frame_2 = series.create_frame(1.0);
    const auto frame_3 = series.create_frame(2.0);
    REQUIRE_RANGE_EQ(series.frames(), {frame_1, frame_2, frame_3});

    // Delete the frame.
    storage.delete_frame(frame_2);
    CHECK_FALSE(storage.check_frame(frame_2));
    CHECK_RANGE_EQ(series.frames(), {frame_1, frame_3});

    // Create more frames.
    // Make sure the ID of the removed frame is not reused.
    const auto frame_4 = series.create_frame(3.0);
    CHECK(storage.check_frame(frame_4));
    CHECK(frame_4 == data::FrameID{4});
    CHECK_RANGE_EQ(series.frames(), {frame_1, frame_3, frame_4});
  }
  SUBCASE("delete series") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");

    // Create a few frames.
    const auto frame_1 = series.create_frame(0.0);
    const auto frame_2 = series.create_frame(1.0);
    const auto frame_3 = series.create_frame(2.0);
    REQUIRE_RANGE_EQ(series.frames(), {frame_1, frame_2, frame_3});

    // Delete the series.
    storage.delete_series(series);
    REQUIRE_FALSE(storage.check_series(series));

    // Make sure the frames are deleted.
    CHECK_FALSE(storage.check_frame(frame_1));
    CHECK_FALSE(storage.check_frame(frame_2));
    CHECK_FALSE(storage.check_frame(frame_3));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::ArrayView") {
  SUBCASE("empty dataset") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);
    CHECK(frame.num_arrays() == 0);
  }
  SUBCASE("create arrays") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);

    const auto array_1 = frame.create_array("array_1");
    array_1.write(std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));
    CHECK(array_1 == data::ArrayID{1});
    CHECK(array_1.name() == "array_1");
    CHECK(array_1.type() == data::type_of<float64_t>);
    CHECK(array_1.size() == 1);
    CHECK_RANGE_EQ(array_1.read<float64_t>(), {std::numbers::pi});
    CHECK(frame.num_arrays() == 1);
    CHECK_RANGE_EQ(frame.arrays(), {array_1});

    const auto array_2 = frame.create_array("array_2");
    array_2.write(data::type_of<float32_t>,
                  to_byte_array(std::numbers::e_v<float32_t>));
    REQUIRE(storage.check_array(array_2));
    CHECK(array_2.name() == "array_2");
    CHECK(array_2 == data::ArrayID{2});
    CHECK(array_2.type() == data::type_of<float32_t>);
    CHECK(array_2.size() == 1);
    CHECK_RANGE_EQ(array_2.read<float32_t>(), {std::numbers::e_v<float32_t>});
    CHECK(frame.num_arrays() == 2);
    CHECK_RANGE_EQ(frame.arrays(), {array_1, array_2});
  }
  SUBCASE("find arrays") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);

    const auto array_1 = frame.create_array("array_1");
    array_1.write(std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 = frame.create_array("array_2");
    array_2.write(std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Find the arrays by name.
    CHECK(frame.find_array("array_1") == array_1);
    CHECK(frame.find_array("array_2") == array_2);
    CHECK_FALSE(frame.find_array("does_not_exist"));
  }
  SUBCASE("update arrays") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);

    const auto array = frame.create_array("array");
    array.write(std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array));
    CHECK(array.size() == 1);
    CHECK_RANGE_EQ(array.read<float64_t>(), {std::numbers::pi});

    // Overwrite the array.
    array.write(std::vector{std::numbers::phi, std::numbers::sqrt3});
    CHECK(array.size() == 2);
    CHECK_RANGE_EQ(array.read<float64_t>(),
                   {std::numbers::phi, std::numbers::sqrt3});
  }
  SUBCASE("delete arrays") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);

    const auto array_1 = frame.create_array("array_1");
    array_1.write(std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 = frame.create_array("array_2");
    array_2.write(std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Delete the arrays.
    storage.delete_array(array_1);
    REQUIRE_FALSE(storage.check_array(array_1));
    CHECK_RANGE_EQ(frame.arrays(), {array_2});

    // Create more arrays. Make sure the ID of the removed array is not reused.
    const auto array_3 = frame.create_array("array_3");
    array_3.write(std::vector{std::numbers::phi});
    CHECK(storage.check_array(array_3));
    CHECK(array_3 == data::ArrayID{3});
    CHECK_RANGE_EQ(frame.arrays(), {array_2, array_3});
  }
  SUBCASE("delete frame") {
    data::Storage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto frame = series.create_frame(0.0);

    const auto array_1 = frame.create_array("array_1");
    array_1.write(std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 = frame.create_array("array_2");
    array_2.write(std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Delete the frame.
    storage.delete_frame(frame);
    REQUIRE_FALSE(storage.check_frame(frame));

    // Make sure the arrays are deleted.
    CHECK_FALSE(storage.check_array(array_1));
    CHECK_FALSE(storage.check_array(array_2));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
