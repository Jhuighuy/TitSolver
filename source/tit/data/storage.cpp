/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/serialization/source_sink.hpp"

#include "tit/data/sqlite.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DataStorage::DataStorage(const std::filesystem::path& path) : db_{path} {
  db_.execute(R"SQL(
    CREATE TABLE IF NOT EXISTS Metadata (
      key         TEXT PRIMARY KEY,
      value       TEXT
    ) STRICT;

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
}

auto DataStorage::path() noexcept -> std::filesystem::path {
  return db_.path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::find_metadata(std::string_view key) const
    -> std::optional<std::string> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT value FROM Metadata WHERE key = ?
  )SQL"};
  statement.bind(key);
  if (statement.step()) return statement.column<std::string>();
  return std::nullopt;
}

auto DataStorage::set_metadata(std::string_view key,
                               std::string_view value) -> void {
  sqlite::Statement statement{db_, R"SQL(
    INSERT OR REPLACE INTO Metadata (key, value) VALUES (?, ?)
  )SQL"};
  statement.run(key, value);
}

auto DataStorage::enumerate_metadata() const
    -> std::vector<std::pair<std::string, std::string>> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT key, value FROM Metadata
  )SQL"};
  std::vector<std::pair<std::string, std::string>> result{};
  while (statement.step()) {
    result.emplace_back(statement.columns<std::string, std::string>());
  }
  return result;
}

void DataStorage::delete_metadata(std::string_view key) {
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM Metadata WHERE key = ?
  )SQL"};
  statement.run(key);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::num_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataSeries
  )SQL"};
  TIT_ASSERT(statement.step(), "Unable to count data series!");
  return statement.column<size_t>();
}

auto DataStorage::series_ids() const -> std::vector<DataSeriesID> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries
  )SQL"};
  std::vector<DataSeriesID> result{};
  while (statement.step()) result.emplace_back(statement.column<uint64_t>());
  return result;
}

auto DataStorage::last_series_id() const -> DataSeriesID {
  TIT_ASSERT(num_series() > 0, "No data series in the storage!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id DESC LIMIT 1
  )SQL"};
  TIT_ASSERT(statement.step(), "Unable to get last data series!");
  return DataSeriesID{statement.column<uint64_t>()};
}

auto DataStorage::create_series_id(std::string_view parameters)
    -> DataSeriesID {
  // Remove the oldest series (series with the lowest ID).
  if (num_series() >= max_series_) {
    sqlite::Statement statement{db_, R"SQL(
      DELETE FROM DataSeries WHERE id IN (
        SELECT id FROM DataSeries ORDER BY id ASC LIMIT -1 OFFSET ?
      )
    )SQL"};
    statement.run(max_series_ - 1);
  }

  // Insert the new series.
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataSeries (parameters) VALUES (?)
  )SQL"};
  statement.run(parameters);
  return DataSeriesID{db_.last_insert_row_id<uint64_t>()};
}

void DataStorage::delete_series(DataSeriesID series_id) {
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
  TIT_ASSERT(statement.step(), "Unable to get series parameters!");
  return statement.column<std::string>();
}

auto DataStorage::series_num_time_steps(DataSeriesID series_id) const
    -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM TimeSteps WHERE series_id = ?
  )SQL"};
  statement.bind(series_id.get());
  TIT_ASSERT(statement.step(), "Unable to count time steps!");
  return statement.column<size_t>();
}

auto DataStorage::series_time_step_ids(DataSeriesID series_id) const
    -> std::vector<DataTimeStepID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM TimeSteps WHERE series_id = ?
  )SQL"};
  statement.bind(series_id.get());
  std::vector<DataTimeStepID> result{};
  while (statement.step()) result.emplace_back(statement.column<uint64_t>());
  return result;
}

auto DataStorage::create_time_step_id(DataSeriesID series_id,
                                      real_t time) -> DataTimeStepID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  const auto uniforms_id = create_set_();
  const auto varyings_id = create_set_();
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO TimeSteps (series_id, time, uniform_id, varying_id)
      VALUES (?, ?, ?, ?)
  )SQL"};
  statement.run(series_id.get(), time, varyings_id.get(), uniforms_id.get());
  const DataTimeStepID time_step_id{db_.last_insert_row_id<uint64_t>()};

  // Associate the data sets with the time step.
  sqlite::Statement asssociate_statement{db_, R"SQL(
    UPDATE DataSets SET time_step_id = ? WHERE id = ?
  )SQL"};
  asssociate_statement.run(time_step_id.get(), varyings_id.get());
  asssociate_statement.run(time_step_id.get(), uniforms_id.get());

  return time_step_id;
}

void DataStorage::delete_time_step(DataTimeStepID time_step_id) {
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
  TIT_ASSERT(statement.step(), "Unable to get time step time!");
  return statement.column<real_t>();
}

auto DataStorage::time_step_uniforms_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT uniform_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  TIT_ASSERT(statement.step(), "Unable to get time step uniform!");
  const DataSetID uniform_id{statement.column<uint64_t>()};
  TIT_ASSERT(check_dataset(uniform_id), "Invalid uniform data set ID!");
  return uniform_id;
}

auto DataStorage::time_step_varyings_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT varying_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  TIT_ASSERT(statement.step(), "Unable to get time step varying!");
  const DataSetID varying_id{statement.column<uint64_t>()};
  TIT_ASSERT(check_dataset(varying_id), "Invalid varying data set ID!");
  return varying_id;
}

auto DataStorage::create_set_() -> DataSetID {
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataSets DEFAULT VALUES
  )SQL"};
  statement.run();
  return DataSetID{db_.last_insert_row_id<uint64_t>()};
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
  TIT_ASSERT(statement.step(), "Unable to count data arrays!");
  return statement.column<size_t>();
}

auto DataStorage::dataset_array_ids(DataSetID dataset_id) const
    -> std::vector<std::pair<std::string, DataArrayID>> {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name, id FROM DataArrays WHERE data_set_id = ?
  )SQL"};
  statement.bind(dataset_id.get());
  std::vector<std::pair<std::string, DataArrayID>> result{};
  while (statement.step()) {
    auto [name, id] = statement.columns<std::string, uint64_t>();
    result.emplace_back(std::move(name), DataArrayID{id});
  }
  return result;
}

auto DataStorage::create_array_id(DataSetID dataset_id,
                                  std::string_view name,
                                  DataType type,
                                  DataSource& data_source) -> DataArrayID {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  TIT_ASSERT(!name.empty(), "Array name must not be empty!");
  TIT_ASSERT(type.known(), "Invalid data type!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataArrays (data_set_id, name, type, data) VALUES (?, ?, ?, ?)
  )SQL"};
  std::vector<byte_t> compressed_data{};
  auto compressed_data_sink =
      make_iter_data_sink(std::back_inserter(compressed_data));
  compressor_.compress(data_source, compressed_data_sink);
  statement.run(dataset_id.get(), name, type.id(), compressed_data);
  return DataArrayID{db_.last_insert_row_id<uint64_t>()};
}

auto DataStorage::create_array_id(DataSetID dataset_id,
                                  std::string_view name,
                                  DataType type,
                                  std::span<const byte_t> data) -> DataArrayID {
  RangeDataSource data_source{data};
  return create_array_id(dataset_id, name, type, data_source);
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
  TIT_ASSERT(statement.step(), "Unable to get data array data type!");
  return DataType{statement.column<uint32_t>()};
}

void DataStorage::read_array_data(DataArrayID array_id,
                                  DataSink& data_sink) const {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT data FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id.get());
  TIT_ASSERT(statement.step(), "Unable to get data array data!");
  const auto compressed_data = statement.column<std::span<const byte_t>>();
  RangeDataSource compressed_data_source{compressed_data};
  decompressor_.decompress(compressed_data_source, data_sink);
}

auto DataStorage::array_data(DataArrayID array_id) const
    -> std::vector<byte_t> {
  std::vector<byte_t> result{};
  auto data_sink = make_iter_data_sink(std::back_inserter(result));
  read_array_data(array_id, data_sink);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
