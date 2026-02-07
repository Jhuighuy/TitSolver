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
#include "tit/core/zstd.hpp"
#include "tit/data/sqlite.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Storage::Storage(const std::filesystem::path& path, bool read_only)
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

auto Storage::path() const -> std::filesystem::path {
  return db_.path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Storage::max_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT max_series FROM Settings
  )SQL"};
  TIT_ENSURE(statement.step(), "Unable to get maximum number of data series!");
  return statement.column<size_t>();
}

void Storage::set_max_series(size_t value) {
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

auto Storage::num_series() const -> size_t {
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataSeries
  )SQL"};
  TIT_ENSURE(statement.step(), "Unable to count data series!");
  return statement.column<size_t>();
}

auto Storage::series_ids() const -> std::generator<SeriesID> {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id ASC
  )SQL"};
  while (statement.step()) co_yield statement.column<SeriesID>();
}

auto Storage::last_series_id() const -> SeriesID {
  TIT_ASSERT(num_series() > 0, "No data series in the storage!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries ORDER BY id DESC LIMIT 1
  )SQL"};
  TIT_ENSURE(statement.step(), "Unable to get last data series!");
  return statement.column<SeriesID>();
}

auto Storage::create_series_id(std::string_view name) -> SeriesID {
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
  return SeriesID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Storage::delete_series(SeriesID series_id) {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataSeries WHERE id = ?
  )SQL"};
  statement.run(series_id);
}

auto Storage::check_series(SeriesID series_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id);
  return statement.step();
}

auto Storage::series_name(SeriesID series_id) const -> std::string {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name FROM DataSeries WHERE id = ?
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to get series name!");
  return statement.column<std::string>();
}

auto Storage::series_num_frames(SeriesID series_id) const -> size_t {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataFrames WHERE series_id = ?
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to count data frames!");
  return statement.column<size_t>();
}

auto Storage::series_frame_ids(SeriesID series_id) const
    -> std::generator<FrameID> {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE series_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(series_id);
  while (statement.step()) co_yield statement.column<FrameID>();
}

auto Storage::series_last_frame_id(SeriesID series_id) const -> FrameID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT(series_num_frames(series_id) > 0, "Series is empty!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE series_id = ? ORDER BY id DESC LIMIT 1
  )SQL"};
  statement.bind(series_id);
  TIT_ENSURE(statement.step(), "Unable to get last time step!");
  return statement.column<FrameID>();
}

auto Storage::series_create_frame_id(SeriesID series_id, float64_t time)
    -> FrameID {
  TIT_ASSERT(check_series(series_id), "Invalid series ID!");
  TIT_ASSERT((series_num_frames(series_id) == 0 ||
              time > series_last_frame(series_id).time()),
             "Frame time must be greater than the last frame time!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataFrames (series_id, time) VALUES (?, ?)
  )SQL"};
  statement.run(series_id, time);
  return FrameID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Storage::delete_frame(FrameID frame_id) {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataFrames WHERE id = ?
  )SQL"};
  statement.run(frame_id);
}

auto Storage::check_frame(FrameID frame_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataFrames WHERE id = ?
  )SQL"};
  statement.bind(frame_id);
  return statement.step();
}

auto Storage::frame_time(FrameID frame_id) const -> float64_t {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT time FROM DataFrames WHERE id = ?
  )SQL"};
  statement.bind(frame_id);
  TIT_ENSURE(statement.step(), "Unable to get frame time!");
  return statement.column<float64_t>();
}

auto Storage::frame_num_arrays(FrameID frame_id) const -> size_t {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT COUNT(*) FROM DataArrays WHERE frame_id = ?
  )SQL"};
  statement.bind(frame_id);
  TIT_ENSURE(statement.step(), "Unable to count data arrays!");
  return statement.column<size_t>();
}

auto Storage::frame_array_ids(FrameID frame_id) const
    -> std::generator<ArrayID> {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE frame_id = ? ORDER BY id ASC
  )SQL"};
  statement.bind(frame_id);
  while (statement.step()) co_yield statement.column<ArrayID>();
}

auto Storage::frame_find_array_id(FrameID frame_id, std::string_view name) const
    -> std::optional<ArrayID> {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE frame_id = ? AND name = ?
  )SQL"};
  statement.bind(frame_id, name);
  if (statement.step()) return ArrayID{statement.column<sqlite::RowID>()};
  return std::nullopt;
}

auto Storage::frame_create_array_id(FrameID frame_id, std::string_view name)
    -> ArrayID {
  TIT_ASSERT(check_frame(frame_id), "Invalid frame ID!");
  TIT_ASSERT(!name.empty(), "Array name must not be empty!");
  TIT_ASSERT(!frame_find_array_id(frame_id, name), "Array already exists!");
  sqlite::Statement statement{db_, R"SQL(
    INSERT INTO DataArrays (frame_id, name) VALUES (?, ?)
  )SQL"};
  statement.run(frame_id, name);
  return ArrayID{db_.last_insert_row_id()};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Storage::delete_array(ArrayID array_id) {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    DELETE FROM DataArrays WHERE id = ?
  )SQL"};
  statement.run(array_id);
}

auto Storage::check_array(ArrayID array_id) const -> bool {
  sqlite::Statement statement{db_, R"SQL(
    SELECT id FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  return statement.step();
}

auto Storage::array_name(ArrayID array_id) const -> std::string {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT name FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array name!");
  return statement.column<std::string>();
}

auto Storage::array_type(ArrayID array_id) const -> Type {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT type FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array data type!");
  return Type{statement.column<uint32_t>()};
}

auto Storage::array_size(ArrayID array_id) const -> size_t {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    SELECT size FROM DataArrays WHERE id = ?
  )SQL"};
  statement.bind(array_id);
  TIT_ENSURE(statement.step(), "Unable to get data array size!");
  return statement.column<size_t>();
}

auto Storage::array_open_write_(ArrayID array_id, Type type, size_t size)
    -> OutputStreamPtr<std::byte> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  sqlite::Statement statement{db_, R"SQL(
    UPDATE DataArrays SET type = ?, size = ? WHERE id = ?
  )SQL"};
  statement.run(type.id(), size, array_id);
  return make_zstd_stream_compressor(
      sqlite::make_blob_writer(db_,
                               "DataArrays",
                               "data",
                               std::to_underlying(array_id)));
}

auto Storage::array_open_read_(ArrayID array_id) const
    -> InputStreamPtr<std::byte> {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  return make_zstd_stream_decompressor(
      sqlite::make_blob_reader(db_,
                               "DataArrays",
                               "data",
                               std::to_underlying(array_id)));
}

void Storage::array_write(ArrayID array_id,
                          Type type,
                          std::span<const std::byte> data) {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  TIT_ASSERT(data.size() % type.width() == 0, "Data size mismatch!");
  array_open_write_(array_id, type, data.size() / type.width())->write(data);
}

void Storage::array_read(ArrayID array_id, std::span<std::byte> data) const {
  TIT_ASSERT(check_array(array_id), "Invalid data array ID!");
  TIT_ASSERT(data.size() == array_size(array_id) * array_type(array_id).width(),
             "Data size mismatch!");
  array_open_read_(array_id)->read(data);
}

auto Storage::array_read(ArrayID array_id) const -> std::vector<std::byte> {
  std::vector<std::byte> result(array_size(array_id) *
                                array_type(array_id).width());
  array_read(array_id, std::span{result});
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
