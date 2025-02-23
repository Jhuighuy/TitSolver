/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"

#include "tit/data/sqlite.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"
#include "tit/data/zstd.hpp"

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
      size        INTEGER,
      data        BLOB,
      FOREIGN KEY (data_set_id) REFERENCES DataSets(id) ON DELETE CASCADE
    ) STRICT;
  )SQL");
  /// @todo We shall check if the database schema is actually what we expect.
}

auto DataStorage::path() const -> std::filesystem::path {
  return db_.path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::max_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT max_series FROM Settings
  )SQL"};
  if (!statement.step()) {
    TIT_THROW("Unable to get maximum number of data series!");
  }
  return statement.column<size_t>();
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
  if (!statement.step()) TIT_THROW("Unable to count data series!");
  return statement.column<size_t>();
}

auto DataStorage::series_ids() const -> InputStreamPtr<DataSeriesID> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id ASC
  )SQL"};
  return make_generator_input_stream<DataSeriesID>(
      [statement = std::move(statement)](DataSeriesID& out) mutable {
        if (!statement.step()) return false;
        out = DataSeriesID{statement.column<sqlite::RowID>()};
        return true;
      });
}

auto DataStorage::last_series_id() const -> DataSeriesID {
  TIT_ASSERT(num_series() > 0, "No data series in the storage!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id DESC LIMIT 1
  )SQL"};
  if (!statement.step()) TIT_THROW("Unable to get last data series!");
  return DataSeriesID{statement.column<sqlite::RowID>()};
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
  if (!statement.step()) TIT_THROW("Unable to get series parameters!");
  return statement.column<std::string>();
}

auto DataStorage::series_num_time_steps(DataSeriesID series_id) const
    -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM TimeSteps WHERE series_id = ?
  )SQL"};
  statement.bind(series_id.get());
  if (!statement.step()) TIT_THROW("Unable to count time steps!");
  return statement.column<size_t>();
}

auto DataStorage::series_time_step_ids(DataSeriesID series_id) const
    -> InputStreamPtr<DataTimeStepID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
        SELECT id FROM TimeSteps WHERE series_id = ? ORDER BY id ASC
      )SQL"};
  statement.bind(series_id.get());
  return make_generator_input_stream<DataTimeStepID>(
      [statement = std::move(statement)](DataTimeStepID& out) mutable {
        if (!statement.step()) return false;
        out = DataTimeStepID{statement.column<sqlite::RowID>()};
        return true;
      });
}

auto DataStorage::series_last_time_step_id(DataSeriesID series_id) const
    -> DataTimeStepID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT(series_num_time_steps(series_id) > 0, "Series is empty!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM TimeSteps WHERE series_id = ? ORDER BY id DESC LIMIT 1
  )SQL"};
  statement.bind(series_id.get());
  if (!statement.step()) TIT_THROW("Unable to get last time step!");
  return DataTimeStepID{statement.column<sqlite::RowID>()};
}

auto DataStorage::create_time_step_id(DataSeriesID series_id, real_t time)
    -> DataTimeStepID {
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
  if (!statement.step()) TIT_THROW("Unable to get time step time!");
  return statement.column<real_t>();
}

auto DataStorage::time_step_uniforms_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT uniform_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  if (!statement.step()) {
    TIT_THROW("Unable to get time step uniform data set ID!");
  }
  return DataSetID{statement.column<sqlite::RowID>()};
}

auto DataStorage::time_step_varyings_id(DataTimeStepID time_step_id) const
    -> DataSetID {
  TIT_ASSERT(check_time_step(time_step_id), "Invalid time step ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT varying_id FROM TimeSteps WHERE id = ?
  )SQL"};
  statement.bind(time_step_id.get());
  if (!statement.step()) {
    TIT_THROW("Unable to get time step varying data set ID!");
  }
  return DataSetID{statement.column<sqlite::RowID>()};
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
  if (!statement.step()) TIT_THROW("Unable to count data arrays!");
  return statement.column<size_t>();
}

auto DataStorage::dataset_array_ids(DataSetID dataset_id) const
    -> InputStreamPtr<std::pair<std::string, DataArrayID>> {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name, id FROM DataArrays WHERE data_set_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(dataset_id.get());
  return make_generator_input_stream<std::pair<std::string, DataArrayID>>(
      [statement = std::move(statement)](
          std::pair<std::string, DataArrayID>& out) mutable {
        if (!statement.step()) return false;
        auto [name, id] = statement.columns<std::string, sqlite::RowID>();
        out = {std::move(name), DataArrayID{id}};
        return true;
      });
}

auto DataStorage::find_array_id(DataSetID dataset_id,
                                std::string_view name) const
    -> std::optional<DataArrayID> {
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
                                  DataType type) -> DataArrayID {
  TIT_ASSERT(check_dataset(dataset_id), "Invalid data set ID!");
  TIT_ASSERT(!name.empty(), "Array name must not be empty!");
  TIT_ASSERT(!find_array_id(dataset_id, name), "Array already exists!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataArrays (data_set_id, name, type) VALUES (?, ?, ?)
  )SQL"};
  statement.run(dataset_id.get(), name, type.id());
  return DataArrayID{db_.last_insert_row_id()};
}

auto DataStorage::create_array_id(DataSetID dataset_id,
                                  std::string_view name,
                                  DataType type,
                                  std::span<const byte_t> data) -> DataArrayID {
  const auto array_id = create_array_id(dataset_id, name, type);
  array_data_open_write(array_id)->write(data);
  return array_id;
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
  if (!statement.step()) TIT_THROW("Unable to get data array data type!");
  return DataType{statement.column<uint32_t>()};
}

auto DataStorage::array_size(DataArrayID array_id) const -> size_t {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT size FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id.get());
  if (!statement.step()) TIT_THROW("Unable to get data array size!");
  return statement.column<size_t>();
}

auto DataStorage::array_data_open_write(DataArrayID array_id)
    -> OutputStreamPtr<byte_t> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  return make_counting_output_stream(
      zstd::make_stream_compressor(
          sqlite::make_blob_writer(db_, "DataArrays", "data", array_id.get())),
      [this, array_id](size_t copied_bytes) {
        const auto byte_width = array_type(array_id).width();
        TIT_ASSERT(copied_bytes % byte_width == 0, "Truncated data!");
        sqlite::Statement statement{db_, R"SQL(
          UPDATE DataArrays SET size = ? WHERE id = ?
        )SQL"};
        statement.run(copied_bytes / byte_width, array_id.get());
      });
}

auto DataStorage::array_data_open_read(DataArrayID array_id) const
    -> InputStreamPtr<byte_t> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  return zstd::make_stream_decompressor(
      sqlite::make_blob_reader(db_, "DataArrays", "data", array_id.get()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
