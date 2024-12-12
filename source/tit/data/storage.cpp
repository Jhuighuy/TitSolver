/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/utils.hpp"

#include "tit/data/sqlite.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DataStorage::DataStorage(const std::filesystem::path& path) : db_{path} {
  db_.execute(R"SQL(
    PRAGMA foreign_keys = ON;

    CREATE TABLE IF NOT EXISTS Settings (
      id INTEGER PRIMARY KEY CHECK (id = 0),
      max_series INTEGER
    ) STRICT;
    INSERT OR IGNORE INTO Settings (id, max_series) VALUES (0, 5);

    CREATE TABLE IF NOT EXISTS DataSeries (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      parameters  TEXT
    ) STRICT;

    CREATE TABLE IF NOT EXISTS TimeSteps (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      series_id   INTEGER NOT NULL,
      time        REAL NOT NULL,
      uniform_id  INTEGER NOT NULL,
      varying_id  INTEGER NOT NULL,
      FOREIGN KEY (series_id) REFERENCES DataSeries(id) ON DELETE CASCADE
    ) STRICT;

    CREATE TABLE IF NOT EXISTS DataSets (
      id INTEGER   PRIMARY KEY AUTOINCREMENT,
      time_step_id INTEGER,
      FOREIGN KEY (time_step_id) REFERENCES TimeSteps(id) ON DELETE CASCADE
    ) STRICT;

    CREATE TABLE IF NOT EXISTS DataArrays (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      data_set_id INTEGER NOT NULL,
      name        TEXT NOT NULL,
      type        INTEGER NOT NULL,
      data        BLOB NOT NULL,
      FOREIGN KEY (data_set_id) REFERENCES DataSets(id) ON DELETE CASCADE
    ) STRICT;
  )SQL");
  /// @todo We shall check if the database schema is actually what we expect.
}

auto DataStorage::path() const -> std::filesystem::path {
  return db_.path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::max_series() -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT max_series FROM Settings
  )SQL"};
  if (statement.step()) return statement.column<size_t>();
  TIT_THROW("Unable to get maximum number of data series!");
}

void DataStorage::set_max_series(size_t value) {
  TIT_ASSERT(value > 0, "Maximum number of data series must be positive!");
  sqlite::Statement update_statement{db_, R"SQL(
    UPDATE Settings SET max_series = ?
  )SQL"};
  update_statement.run(value);
  if (num_series() > value) {
    sqlite::Statement remove_extra_statement{db_, R"SQL(
      DELETE FROM DataSeries WHERE id IN (
        SELECT id FROM DataSeries ORDER BY id ASC LIMIT ?
      )
    )SQL"};
    remove_extra_statement.run(num_series() - value);
  }
}

auto DataStorage::num_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataSeries
  )SQL"};
  if (statement.step()) return statement.column<size_t>();
  TIT_THROW("Unable to count data series!");
}

auto DataStorage::series_ids() const -> std::vector<DataSeriesID> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id ASC
  )SQL"};
  std::vector<DataSeriesID> result{};
  while (statement.step()) {
    result.emplace_back(statement.column<sqlite::RowID>());
  }
  return result;
}

auto DataStorage::last_series_id() const -> DataSeriesID {
  TIT_ASSERT(num_series() > 0, "No data series in the storage!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id DESC LIMIT 1
  )SQL"};
  if (statement.step()) return DataSeriesID{statement.column<sqlite::RowID>()};
  TIT_THROW("Unable to get last data series!");
}

auto DataStorage::create_series_id(std::string_view parameters)
    -> DataSeriesID {
  if (num_series() >= max_series()) {
    // Delete the oldest series if the maximum number of series is reached.
    db_.execute(R"SQL(
      DELETE FROM DataSeries WHERE id IN (
        SELECT id FROM DataSeries ORDER BY id ASC LIMIT 1
      )
    )SQL");
  }
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataSeries (parameters) VALUES (?)
  )SQL"};
  statement.run(parameters);
  return DataSeriesID{db_.last_insert_row_id()};
}

void DataStorage::delete_series(DataSeriesID series_id) {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataSeries WHERE id = ?
  )SQL"};
  statement.run(series_id.get());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::check_series(DataSeriesID series_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id.get());
  return statement.step();
}

auto DataStorage::series_parameters(DataSeriesID series_id) const
    -> std::string {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT parameters FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id.get());
  if (statement.step()) return statement.column<std::string>();
  TIT_THROW("Unable to get series parameters!");
}

auto DataStorage::series_num_time_steps(DataSeriesID series_id) const
    -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM TimeSteps WHERE series_id = ?
  )SQL"};
  statement.bind(series_id.get());
  if (statement.step()) return statement.column<size_t>();
  TIT_THROW("Unable to count time steps!");
}

auto DataStorage::series_time_step_ids(DataSeriesID series_id) const
    -> std::vector<DataTimeStepID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM TimeSteps WHERE series_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(series_id.get());
  std::vector<DataTimeStepID> result{};
  while (statement.step()) {
    result.emplace_back(statement.column<sqlite::RowID>());
  }
  return result;
}

auto DataStorage::series_last_time_step_id(DataSeriesID series_id) const
    -> DataTimeStepID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT(series_num_time_steps(series_id) > 0, "Series is empty!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM TimeSteps WHERE series_id = ? ORDER BY id DESC LIMIT 1
  )SQL"};
  statement.bind(series_id.get());
  if (statement.step()) {
    return DataTimeStepID{statement.column<sqlite::RowID>()};
  }
  TIT_THROW("Unable to get last time step!");
}

auto DataStorage::create_time_step_id(DataSeriesID series_id,
                                      real_t time) -> DataTimeStepID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT((series_num_time_steps(series_id) == 0 ||
              time > series_last_time_step(series_id).time()),
             "Time step time must be greater than the last time step!");

  const auto uniforms_id = create_set_();
  const auto varyings_id = create_set_();
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO TimeSteps (series_id, time, uniform_id, varying_id)
      VALUES (?, ?, ?, ?)
  )SQL"};
  statement.run(series_id.get(), time, uniforms_id.get(), varyings_id.get());
  const DataTimeStepID time_step_id{db_.last_insert_row_id()};

  // Associate the data sets with the time step.
  sqlite::Statement associate_statement{db_, R"SQL(
    UPDATE DataSets SET time_step_id = ? WHERE id = ?
  )SQL"};
  associate_statement.run(time_step_id.get(), uniforms_id.get());
  associate_statement.run(time_step_id.get(), varyings_id.get());

  return time_step_id;
}

void DataStorage::delete_time_step(DataTimeStepID time_step_id) {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.run(time_step_id.get());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::check_time_step(DataTimeStepID time_step_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  return statement.step();
}

auto DataStorage::time_step_time(DataTimeStepID time_step_id) const -> real_t {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT time FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  if (statement.step()) return statement.column<real_t>();
  TIT_THROW("Unable to get time step time!");
}

auto DataStorage::time_step_uniforms_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT uniform_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  if (statement.step()) return DataSetID{statement.column<sqlite::RowID>()};
  TIT_THROW("Unable to get time step uniform data set ID!");
}

auto DataStorage::time_step_varyings_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT varying_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  if (statement.step()) return DataSetID{statement.column<sqlite::RowID>()};
  TIT_THROW("Unable to get time step varying data set ID!");
}

auto DataStorage::create_set_() -> DataSetID {
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataSets DEFAULT VALUES
  )SQL"};
  statement.run();
  return DataSetID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::check_dataset(DataSetID dataset_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSets WHERE id = ?
  )SQL"};
  statement.bind(dataset_id.get());
  return statement.step();
}

auto DataStorage::dataset_num_arrays(DataSetID dataset_id) const -> size_t {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataArrays WHERE data_set_id = ?
  )SQL"};
  statement.bind(dataset_id.get());
  if (statement.step()) return statement.column<size_t>();
  TIT_THROW("Unable to count data arrays!");
}

auto DataStorage::dataset_array_ids(DataSetID dataset_id) const
    -> std::vector<std::pair<std::string, DataArrayID>> {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name, id FROM DataArrays WHERE data_set_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(dataset_id.get());
  std::vector<std::pair<std::string, DataArrayID>> result{};
  while (statement.step()) {
    auto [name, id] = statement.columns<std::string, sqlite::RowID>();
    result.emplace_back(std::move(name), DataArrayID{id});
  }
  return result;
}

auto DataStorage::find_array_id(DataSetID dataset_id, std::string_view name)
    const -> std::optional<DataArrayID> {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE data_set_id = ? AND name = ?
  )SQL"};
  statement.bind(dataset_id.get(), name);
  if (statement.step()) return DataArrayID{statement.column<sqlite::RowID>()};
  return std::nullopt;
}

auto DataStorage::create_array_id(DataSetID dataset_id,
                                  std::string_view name,
                                  DataType type,
                                  ByteSpan data) -> DataArrayID {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  TIT_ASSERT(!name.empty(), "Array name must not be empty!");
  TIT_ASSERT(!find_array_id(dataset_id, name), "Array already exists!");
  TIT_ASSERT(type.known(), "Invalid data type!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataArrays (data_set_id, name, type, data) VALUES (?, ?, ?, ?)
  )SQL"};
  statement.run(dataset_id.get(), name, type.id(), data);
  return DataArrayID{db_.last_insert_row_id()};
}

void DataStorage::delete_array(DataArrayID array_id) {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataArrays WHERE id = ?
  )SQL"};
  statement.run(array_id.get());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::check_array(DataArrayID array_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id.get());
  return statement.step();
}

auto DataStorage::array_type(DataArrayID array_id) const -> DataType {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT type FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id.get());
  if (statement.step()) return DataType{statement.column<uint32_t>()};
  TIT_THROW("Unable to get data array data type!");
}

auto DataStorage::array_data(DataArrayID array_id) const -> Bytes {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT data FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id.get());
  if (statement.step()) return statement.column<Bytes>();
  TIT_THROW("Unable to get data array data!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
