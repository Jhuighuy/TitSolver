/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <numbers>
#include <set>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/range_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataStorage") {
  const std::filesystem::path file_name{"test.ttdb"};
  const std::filesystem::path invalid_file_name{"/invalid/path/to/file.ttdb"};
  SUBCASE("success") {
    SUBCASE("create in-memory") {
      const data::DataStorage storage{":memory:"};
      CHECK(storage.path().empty());
    }
    SUBCASE("create file") {
      if (std::filesystem::exists(file_name)) {
        // Remove the file if it already exists.
        REQUIRE(std::filesystem::remove(file_name));
      }
      const data::DataStorage storage{file_name};
      CHECK(std::filesystem::exists(file_name));
      CHECK(storage.path().filename() == file_name);
    }
    SUBCASE("open existing") {
      // Should exist due to the previous test.
      REQUIRE(std::filesystem::exists(file_name));
      const data::DataStorage storage{file_name};
      CHECK(storage.path().filename() == file_name);
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
      // Our current executable definitely is not a valid `.ttdb` file.
      const data::sqlite::Database db{exe_path()};
      CHECK_THROWS_MSG(db.execute("SELECT * FROM test"),
                       Exception,
                       "file is not a database");
    }
    /// @todo We shall add tests for valid database files with invalid schema.
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataSeriesView") {
  SUBCASE("empty storage") {
    const data::DataStorage storage{":memory:"};
    CHECK(storage.num_series() == 0);
    CHECK(storage.series_ids().empty());
  }
  SUBCASE("create series") {
    data::DataStorage storage{":memory:"};
    REQUIRE(storage.max_series() >= 3);

    const auto series_1 = storage.create_series("1");
    REQUIRE(storage.check_series(series_1));
    CHECK(series_1 == data::DataSeriesID{1});
    CHECK(series_1.parameters() == "1");
    CHECK(storage.num_series() == 1);
    CHECK_RANGE_EQ(storage.series(), {series_1});
    CHECK(storage.last_series() == series_1);

    const auto series_2 = storage.create_series("2");
    REQUIRE(storage.check_series(series_2));
    CHECK(series_2 == data::DataSeriesID{2});
    CHECK(series_2.parameters() == "2");
    CHECK(storage.num_series() == 2);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_2});
    CHECK(storage.last_series() == series_2);

    const auto series_3 = storage.create_series("3");
    REQUIRE(storage.check_series(series_3));
    CHECK(series_3 == data::DataSeriesID{3});
    CHECK(series_3.parameters() == "3");
    CHECK(storage.num_series() == 3);
    CHECK_RANGE_EQ(storage.series(), {series_1, series_2, series_3});
    CHECK(storage.last_series() == series_3);
  }
  SUBCASE("create more series than the maximum") {
    data::DataStorage storage{":memory:"};
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
    data::DataStorage storage{":memory:"};
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
    data::DataStorage storage{":memory:"};
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
    data::DataStorage storage{":memory:"};
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

TEST_CASE("data::DataTimeStepView") {
  SUBCASE("empty series") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    CHECK(series.num_time_steps() == 0);
    CHECK(series.time_steps().empty());
  }
  SUBCASE("create time steps") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");

    const auto step_1 = series.create_time_step(0.0);
    REQUIRE(storage.check_time_step(step_1));
    CHECK(step_1 == data::DataTimeStepID{1});
    CHECK(step_1.time() == 0.0);
    CHECK(storage.check_dataset(step_1.uniforms()));
    CHECK(storage.check_dataset(step_1.varyings()));
    CHECK(series.num_time_steps() == 1);
    CHECK_RANGE_EQ(series.time_steps(), {step_1});
    CHECK(series.last_time_step() == step_1);

    const auto step_2 = series.create_time_step(1.0);
    REQUIRE(storage.check_time_step(step_2));
    CHECK(step_2 == data::DataTimeStepID{2});
    CHECK(step_2.time() == 1.0);
    CHECK(storage.check_dataset(step_2.uniforms()));
    CHECK(storage.check_dataset(step_2.varyings()));
    CHECK(series.num_time_steps() == 2);
    CHECK_RANGE_EQ(series.time_steps(), {step_1, step_2});
    CHECK(series.last_time_step() == step_2);

    const auto step_3 = series.create_time_step(2.0);
    REQUIRE(storage.check_time_step(step_3));
    CHECK(step_3 == data::DataTimeStepID{3});
    CHECK(step_3.time() == 2.0);
    CHECK(storage.check_dataset(step_3.uniforms()));
    CHECK(storage.check_dataset(step_3.varyings()));
    CHECK(series.num_time_steps() == 3);
    CHECK_RANGE_EQ(series.time_steps(), {step_1, step_2, step_3});
    CHECK(series.last_time_step() == step_3);
  }
  SUBCASE("create time steps in different series") {
    data::DataStorage storage{":memory:"};
    const auto series_1 = storage.create_series("");
    const auto series_2 = storage.create_series("");

    // Create time steps in the first series.
    const auto step_11 = series_1.create_time_step(0.0);
    const auto step_12 = series_1.create_time_step(1.0);
    const auto step_13 = series_1.create_time_step(2.0);
    REQUIRE_RANGE_EQ(series_1.time_steps(), {step_11, step_12, step_13});

    // Create time steps in the second series.
    const auto step_21 = series_2.create_time_step(0.0);
    const auto step_22 = series_2.create_time_step(1.0);
    const auto step_23 = series_2.create_time_step(2.0);
    REQUIRE_RANGE_EQ(series_2.time_steps(), {step_21, step_22, step_23});

    // Make sure the time steps are not shared between series.
    const std::set<data::DataTimeStepID> all_steps{
        step_11,
        step_12,
        step_13,
        step_21,
        step_22,
        step_23,
    };
    CHECK(all_steps.size() == 6);
  }
  SUBCASE("delete time steps") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");

    // Create a few time steps.
    const auto step_1 = series.create_time_step(0.0);
    const auto step_2 = series.create_time_step(1.0);
    const auto step_3 = series.create_time_step(2.0);
    REQUIRE_RANGE_EQ(series.time_steps(), {step_1, step_2, step_3});

    // Delete the time step.
    storage.delete_time_step(step_2);
    CHECK_FALSE(storage.check_time_step(step_2));
    CHECK_RANGE_EQ(series.time_steps(), {step_1, step_3});

    // Create more time steps.
    // Make sure the ID of the removed time step is not reused.
    const auto step_4 = series.create_time_step(3.0);
    CHECK(storage.check_time_step(step_4));
    CHECK(step_4 == data::DataTimeStepID{4});
    CHECK_RANGE_EQ(series.time_steps(), {step_1, step_3, step_4});
  }
  SUBCASE("delete series") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");

    // Create a few time steps.
    const auto step_1 = series.create_time_step(0.0);
    const auto step_2 = series.create_time_step(1.0);
    const auto step_3 = series.create_time_step(2.0);
    REQUIRE_RANGE_EQ(series.time_steps(), {step_1, step_2, step_3});

    // Delete the series.
    storage.delete_series(series);
    REQUIRE_FALSE(storage.check_series(series));

    // Make sure the time steps are deleted.
    CHECK_FALSE(storage.check_time_step(step_1));
    CHECK_FALSE(storage.check_time_step(step_2));
    CHECK_FALSE(storage.check_time_step(step_3));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataSetView") {
  // Data sets are created by the time step. There is no way to create or delete
  // a data set without a time step.
  SUBCASE("create time step") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);

    const auto uniforms = step.uniforms();
    REQUIRE(storage.check_dataset(uniforms));
    CHECK(uniforms == data::DataSetID{1});
    CHECK(uniforms.num_arrays() == 0);
    CHECK(uniforms.arrays().empty());

    const auto varyings = step.varyings();
    REQUIRE(storage.check_dataset(varyings));
    CHECK(varyings == data::DataSetID{2});
    CHECK(varyings.num_arrays() == 0);
    CHECK(varyings.arrays().empty());
  }
  SUBCASE("datasets in different time steps") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step_1 = series.create_time_step(0.0);
    const auto step_2 = series.create_time_step(1.0);

    const auto uniforms_1 = step_1.uniforms();
    REQUIRE(storage.check_dataset(uniforms_1));
    const auto varyings_1 = step_1.varyings();
    REQUIRE(storage.check_dataset(varyings_1));

    const auto uniforms_2 = step_2.uniforms();
    REQUIRE(storage.check_dataset(uniforms_2));
    const auto varyings_2 = step_2.varyings();
    REQUIRE(storage.check_dataset(varyings_2));

    // Make sure the datasets are not shared between time steps.
    const std::set<data::DataSetID> all_datasets{
        uniforms_1,
        varyings_1,
        uniforms_2,
        varyings_2,
    };
    CHECK(all_datasets.size() == 4);
  }
  SUBCASE("delete time step") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);

    const auto uniforms = step.uniforms();
    REQUIRE(storage.check_dataset(uniforms));

    const auto varyings = step.varyings();
    REQUIRE(storage.check_dataset(varyings));

    // Delete the time step.
    storage.delete_time_step(step);
    REQUIRE_FALSE(storage.check_time_step(step));

    // Make sure the datasets are deleted.
    CHECK_FALSE(storage.check_dataset(uniforms));
    CHECK_FALSE(storage.check_dataset(varyings));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::DataArrayView") {
  SUBCASE("empty dataset") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);
    const auto dataset = step.uniforms();
    CHECK(dataset.num_arrays() == 0);
    CHECK(dataset.arrays().empty());
  }
  SUBCASE("create arrays") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);
    const auto dataset = step.uniforms();

    const auto array_1 = dataset.create_array( //
        "array_1",
        std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));
    CHECK(array_1 == data::DataArrayID{1});
    CHECK(array_1.type() == data::type_of<float64_t>);
    CHECK_RANGE_EQ(array_1.template data<float64_t>(), {std::numbers::pi});
    CHECK(dataset.num_arrays() == 1);
    CHECK_RANGE_EQ(dataset.arrays(), {{"array_1", array_1}});

    const auto array_2 = dataset.create_array( //
        "array_2",
        data::type_of<float32_t>,
        to_byte_array(std::numbers::e_v<float32_t>));
    REQUIRE(storage.check_array(array_2));
    CHECK(array_2 == data::DataArrayID{2});
    CHECK(array_2.type() == data::type_of<float32_t>);
    CHECK_RANGE_EQ(array_2.data(), to_byte_array(std::numbers::e_v<float32_t>));
    CHECK(dataset.num_arrays() == 2);
    CHECK_RANGE_EQ(dataset.arrays(),
                   {{"array_1", array_1}, {"array_2", array_2}});
  }
  SUBCASE("find arrays") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);
    const auto dataset = step.uniforms();

    const auto array_1 =
        dataset.create_array("array_1", std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 =
        dataset.create_array("array_2", std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Find the arrays by name.
    CHECK(dataset.find_array("array_1") == array_1);
    CHECK(dataset.find_array("array_2") == array_2);
    CHECK_FALSE(dataset.find_array("does_not_exist"));
  }
  SUBCASE("delete arrays") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);
    const auto dataset = step.uniforms();

    const auto array_1 =
        dataset.create_array("array_1", std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 =
        dataset.create_array("array_2", std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Delete the arrays.
    storage.delete_array(array_1);
    REQUIRE_FALSE(storage.check_array(array_1));
    CHECK_RANGE_EQ(dataset.arrays(), {{"array_2", array_2}});

    // Create more arrays. Make sure the ID of the removed array is not reused.
    const auto array_3 =
        dataset.create_array("array_3", std::vector{std::numbers::phi});
    CHECK(storage.check_array(array_3));
    CHECK(array_3 == data::DataArrayID{3});
    CHECK_RANGE_EQ(dataset.arrays(),
                   {{"array_2", array_2}, {"array_3", array_3}});
  }
  SUBCASE("delete time step") {
    data::DataStorage storage{":memory:"};
    const auto series = storage.create_series("");
    const auto step = series.create_time_step(0.0);
    const auto dataset = step.uniforms();

    const auto array_1 =
        dataset.create_array("array_1", std::vector{std::numbers::pi});
    REQUIRE(storage.check_array(array_1));

    const auto array_2 =
        dataset.create_array("array_2", std::vector{std::numbers::e});
    REQUIRE(storage.check_array(array_2));

    // Delete the time step.
    storage.delete_time_step(step);
    REQUIRE_FALSE(storage.check_time_step(step));

    // Make sure the arrays are deleted.
    CHECK_FALSE(storage.check_array(array_1));
    CHECK_FALSE(storage.check_array(array_2));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
