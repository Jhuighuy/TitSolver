/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <filesystem>
#include <generator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/param_spec.hpp"
#include "tit/data/sqlite.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"
#include "tit/data/zstd.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DataStorage::DataStorage(const std::filesystem::path& path, bool read_only)
    : db_{path, read_only} {
  if (read_only) return;
  db_.execute(R"SQL(
    PRAGMA journal_mode = WAL;
    PRAGMA foreign_keys = ON;

    CREATE TABLE IF NOT EXISTS Settings (
      id INTEGER PRIMARY KEY CHECK (id = 0),
      max_series INTEGER
    ) STRICT;
    INSERT OR IGNORE INTO Settings (id, max_series) VALUES (0, 5);

    CREATE TABLE IF NOT EXISTS DataSeries (
      id   INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL
    ) STRICT;

    CREATE TABLE IF NOT EXISTS DataParams (
      id        INTEGER PRIMARY KEY AUTOINCREMENT,
      series_id INTEGER NOT NULL,
      parent_id INTEGER,
      spec      TEXT NOT NULL,
      value     TEXT NOT NULL,
      FOREIGN KEY (series_id) REFERENCES DataSeries(id) ON DELETE CASCADE,
      FOREIGN KEY (parent_id) REFERENCES DataParams(id) ON DELETE CASCADE
    ) STRICT;

    CREATE TABLE IF NOT EXISTS DataFrames (
      id        INTEGER PRIMARY KEY AUTOINCREMENT,
      series_id INTEGER NOT NULL,
      time      REAL NOT NULL,
      FOREIGN KEY (series_id) REFERENCES DataSeries(id) ON DELETE CASCADE
    ) STRICT;

    CREATE TABLE IF NOT EXISTS DataArrays (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      frame_id INTEGER NOT NULL,
      name     TEXT NOT NULL,
      type     INTEGER,
      size     INTEGER,
      data     BLOB,
      FOREIGN KEY (frame_id) REFERENCES DataFrames(id) ON DELETE CASCADE
    ) STRICT;
  )SQL");
}

auto DataStorage::path() const -> std::filesystem::path {
  return db_.path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto DataStorage::max_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT max_series FROM Settings
  )SQL"};
  TIT_ENSURE(statement.step(), "Unable to get maximum number of data series!");
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
  TIT_ENSURE(statement.step(), "Unable to count data series!");
  return statement.column<size_t>();
}

auto DataStorage::series_ids() const -> std::generator<DataSeriesID> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id ASC
  )SQL"};
  while (statement.step()) co_yield statement.column<DataSeriesID>();
}

auto DataStorage::last_series_id() const -> DataSeriesID {
  TIT_ASSERT(num_series() > 0, "No data series in the storage!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id DESC LIMIT 1
  )SQL"};
  TIT_ENSURE(statement.step(), "Unable to get last data series!");
  return statement.column<DataSeriesID>();
}

auto DataStorage::create_series_id(std::string_view name) -> DataSeriesID {
  if (num_series() >= max_series()) {
    // Delete the oldest series if the maximum number of series is reached.
    db_.execute(R"SQL(
      DELETE FROM DataSeries WHERE id IN (
        SELECT id FROM DataSeries ORDER BY id ASC LIMIT 1
      )
    )SQL");
  }
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataSeries (name) VALUES (?)
  )SQL"};
  statement.run(name);
  return DataSeriesID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void DataStorage::delete_series(DataSeriesID series_id) {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataSeries WHERE id = ?
  )SQL"};
  statement.run(series_id);
}

auto DataStorage::check_series(DataSeriesID series_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id);
  return statement.step();
}

auto DataStorage::series_name(DataSeriesID series_id) const -> std::string {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to get series name!");
  return statement.column<std::string>();
}

auto DataStorage::series_num_params(DataSeriesID series_id) const -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataParams WHERE series_id = ?
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to count data parameters!");
  return statement.column<size_t>();
}

auto DataStorage::series_param_ids(DataSeriesID series_id) const
    -> std::generator<DataParamID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id
    FROM DataParams
    WHERE series_id = ?
    ORDER BY id ASC
  )SQL"};
  statement.bind(series_id);
  while (statement.step()) co_yield statement.column<DataParamID>();
}

auto DataStorage::series_create_param_id(DataSeriesID series_id,
                                         const ParamSpec& spec,
                                         DataParamID parent_id,
                                         std::string_view value)
    -> DataParamID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  if (parent_id == DataParamID{0}) {
    sqlite::Statement statement{db_, R"SQL(
      INSERT INTO DataParams (series_id, spec, value) VALUES (?, ?, ?)
    )SQL"};
    statement.run(series_id, spec.to_string(), value);
  } else {
    TIT_ASSERT(check_param(parent_id), "Invalid parent parameter ID!");
    TIT_ENSURE(param_spec(parent_id)->type() == ParamSpecType::record,
               "Parent parameter must be a record!");
    sqlite::Statement statement{db_, R"SQL(
      INSERT INTO DataParams (series_id, parent_id, spec, value)
        VALUES (?, ?, ?, ?)
    )SQL"};
    statement.run(series_id, parent_id, spec.to_string(), value);
  }
  return DataParamID{db_.last_insert_row_id()};
}

auto DataStorage::series_num_frames(DataSeriesID series_id) const -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataFrames WHERE series_id = ?
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to count data frames!");
  return statement.column<size_t>();
}

auto DataStorage::series_frame_ids(DataSeriesID series_id) const
    -> std::generator<DataFrameID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE series_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(series_id);
  while (statement.step()) co_yield statement.column<DataFrameID>();
}

auto DataStorage::series_last_frame_id(DataSeriesID series_id) const
    -> DataFrameID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT(series_num_frames(series_id) > 0, "Series is empty!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE series_id = ? ORDER BY id DESC LIMIT 1
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to get last time step!");
  return statement.column<DataFrameID>();
}

auto DataStorage::series_create_frame_id(DataSeriesID series_id, float64_t time)
    -> DataFrameID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT((series_num_frames(series_id) == 0 ||
              time > series_last_frame(series_id).time()),
             "Frame time must be greater than the last frame time!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataFrames (series_id, time) VALUES (?, ?)
  )SQL"};
  statement.run(series_id, time);
  return DataFrameID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void DataStorage::delete_param(DataParamID param_id) {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataParams WHERE id = ?
  )SQL"};
  statement.run(param_id);
}

auto DataStorage::check_param(DataParamID param_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataParams WHERE id = ?
  )SQL"};
  statement.bind(param_id);
  return statement.step();
}

auto DataStorage::param_spec(DataParamID param_id) const -> ParamSpecPtr {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT spec FROM DataParams WHERE id = ?
  )SQL"};
  statement.bind(param_id);
  TIT_ENSURE(statement.step(), "Unable to get parameter specification!");
  return ParamSpec::from_string(statement.column<std::string>());
}

auto DataStorage::param_value(DataParamID param_id) const -> std::string {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT value FROM DataParams WHERE id = ?
  )SQL"};
  statement.bind(param_id);
  TIT_ENSURE(statement.step(), "Unable to get parameter value!");
  return statement.column<std::string>();
}

void DataStorage::param_set_value(DataParamID param_id,
                                  std::string_view value) {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  param_spec(param_id)->validate(value);
  sqlite::Statement statement{db_, R"SQL(
    UPDATE DataParams SET value = ? WHERE id = ?
  )SQL"};
  statement.run(value, param_id);
}

auto DataStorage::param_parent_id(DataParamID param_id) const -> DataParamID {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT parent_id FROM DataParams WHERE id = ?
  )SQL"};
  statement.bind(param_id);
  TIT_ENSURE(statement.step(), "Unable to get parameter parent!");
  return DataParamID{statement.column<sqlite::RowID>()};
}

auto DataStorage::param_num_children(DataParamID param_id) const -> size_t {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataParams WHERE parent_id = ?
  )SQL"};
  statement.bind(param_id);
  TIT_ENSURE(statement.step(), "Unable to count parameter children!");
  return statement.column<size_t>();
}

auto DataStorage::param_child_ids(DataParamID param_id) const
    -> std::generator<DataParamID> {
  TIT_ASSERT(check_param(param_id), "Invalid parameter ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataParams WHERE parent_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(param_id);
  while (statement.step()) co_yield statement.column<DataParamID>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void DataStorage::delete_frame(DataFrameID frame_id) {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataFrames WHERE id = ?
  )SQL"};
  statement.run(frame_id);
}

auto DataStorage::check_frame(DataFrameID frame_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE id = ?
  )SQL"};
  statement.bind(frame_id);
  return statement.step();
}

auto DataStorage::frame_time(DataFrameID frame_id) const -> float64_t {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT time FROM DataFrames WHERE id = ?
  )SQL"};
  statement.bind(frame_id);
  TIT_ENSURE(statement.step(), "Unable to get frame time!");
  return statement.column<float64_t>();
}

auto DataStorage::frame_num_arrays(DataFrameID frame_id) const -> size_t {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataArrays WHERE frame_id = ?
  )SQL"};
  statement.bind(frame_id);
  TIT_ENSURE(statement.step(), "Unable to count data arrays!");
  return statement.column<size_t>();
}

auto DataStorage::frame_array_ids(DataFrameID frame_id) const
    -> std::generator<DataArrayID> {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE frame_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(frame_id);
  while (statement.step()) co_yield statement.column<DataArrayID>();
}

auto DataStorage::frame_find_array_id(DataFrameID frame_id,
                                      std::string_view name) const
    -> std::optional<DataArrayID> {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE frame_id = ? AND name = ?
  )SQL"};
  statement.bind(frame_id, name);
  if (statement.step()) return DataArrayID{statement.column<sqlite::RowID>()};
  return std::nullopt;
}

auto DataStorage::frame_create_array_id(DataFrameID frame_id,
                                        std::string_view name) -> DataArrayID {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  TIT_ASSERT(!name.empty(), "Array name must not be empty!");
  TIT_ASSERT(!frame_find_array_id(frame_id, name), "Array already exists!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataArrays (frame_id, name) VALUES (?, ?)
  )SQL"};
  statement.run(frame_id, name);
  return DataArrayID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void DataStorage::delete_array(DataArrayID array_id) {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataArrays WHERE id = ?
  )SQL"};
  statement.run(array_id);
}

auto DataStorage::check_array(DataArrayID array_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  return statement.step();
}

auto DataStorage::array_name(DataArrayID array_id) const -> std::string {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array name!");
  return statement.column<std::string>();
}

auto DataStorage::array_type(DataArrayID array_id) const -> DataType {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT type FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array data type!");
  return DataType{statement.column<uint32_t>()};
}

auto DataStorage::array_size(DataArrayID array_id) const -> size_t {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT size FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array size!");
  return statement.column<size_t>();
}

auto DataStorage::array_open_write_(DataArrayID array_id,
                                    DataType type,
                                    size_t size) -> OutputStreamPtr<std::byte> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    UPDATE DataArrays SET type = ?, size = ? WHERE id = ?
  )SQL"};
  statement.run(type.id(), size, array_id);
  return zstd::make_stream_compressor(
      sqlite::make_blob_writer(db_,
                               "DataArrays",
                               "data",
                               std::to_underlying(array_id)));
}

auto DataStorage::array_open_read_(DataArrayID array_id) const
    -> InputStreamPtr<std::byte> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  return zstd::make_stream_decompressor(
      sqlite::make_blob_reader(db_,
                               "DataArrays",
                               "data",
                               std::to_underlying(array_id)));
}

void DataStorage::array_write(DataArrayID array_id,
                              DataType type,
                              std::span<const std::byte> data) {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  TIT_ASSERT(data.size() % type.width() == 0, "Data size mismatch!");
  array_open_write_(array_id, type, data.size() / type.width())->write(data);
}

void DataStorage::array_read(DataArrayID array_id,
                             std::span<std::byte> data) const {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  TIT_ASSERT(data.size() == array_size(array_id) * array_type(array_id).width(),
             "Data size mismatch!");
  array_open_read_(array_id)->read(data);
}

auto DataStorage::array_read(DataArrayID array_id) const
    -> std::vector<std::byte> {
  std::vector<std::byte> result(array_size(array_id) *
                                array_type(array_id).width());
  array_read(array_id, std::span{result});
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
