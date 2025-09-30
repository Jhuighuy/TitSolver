/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <filesystem>
#include <generator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/sqlite.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data series ID type.
enum class DataSeriesID : sqlite::RowID {};

/// Data frame ID type.
enum class DataFrameID : sqlite::RowID {};

/// Data array ID type.
enum class DataArrayID : sqlite::RowID {};

/// Data storage type.
template<class Storage>
concept data_storage =
    std::same_as<std::remove_const_t<Storage>, class DataStorage>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data array view.
template<data_storage Storage>
class DataArrayView final {
public:

  /// Construct a null data array view.
  constexpr DataArrayView() noexcept = default;

  /// Construct a data array view.
  constexpr explicit DataArrayView(Storage& storage, DataArrayID array_id)
      : storage_{&storage}, array_id_{array_id} {
    TIT_ASSERT(storage.check_array(array_id_), "Invalid data array ID!");
  }

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the data array ID.
  /// @{
  constexpr auto id() const noexcept -> DataArrayID {
    TIT_ASSERT(array_id_ != DataArrayID{0}, "Array ID is null!");
    return array_id_;
  }
  constexpr explicit(false) operator DataArrayID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data array views by ID.
  friend constexpr auto operator==(DataArrayView lhs,
                                   DataArrayView rhs) noexcept -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.array_id_ == rhs.array_id_;
  }

  /// Get the name of the data array.
  auto name() const -> std::string {
    return storage().array_name(array_id_);
  }

  /// Get the data type of the data array.
  auto type() const -> DataType {
    return storage().array_type(array_id_);
  }

  /// Get the size of a data array (in elements).
  auto size() const -> size_t {
    return storage().array_size(array_id_);
  }

  /// Open an output stream to write the data.
  auto open_write(DataType type, size_t size) const -> OutputStreamPtr<byte_t>
    requires (!std::is_const_v<Storage>)
  {
    return storage().array_data_open_write(array_id_, type, size);
  }

  /// Write data to the data array.
  template<known_type_of Val>
  void write(std::span<const Val> data) const {
    storage().array_write(array_id_, data);
  }

  /// Open an input stream to read the data.
  /// @{
  auto open_read() const -> InputStreamPtr<byte_t> {
    return storage().array_open_read(array_id_);
  }
  template<known_type_of Val>
  auto open_read() const -> InputStreamPtr<Val> {
    return storage().template array_open_read<Val>(array_id_);
  }
  /// @}

private:

  Storage* storage_ = nullptr;
  DataArrayID array_id_{0};

}; // class DataArrayView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data frame view.
template<data_storage Storage>
class DataFrameView final {
public:

  /// Construct a null data frame view.
  constexpr DataFrameView() noexcept = default;

  /// Construct a data frame view.
  constexpr explicit DataFrameView(Storage& storage, DataFrameID frame_id)
      : storage_{&storage}, frame_id_{frame_id} {
    TIT_ASSERT(storage.check_frame(frame_id_), "Invalid frame ID!");
  }

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the frame ID.
  /// @{
  constexpr auto id() const noexcept -> DataFrameID {
    TIT_ASSERT(frame_id_ != DataFrameID{0}, "Frame ID is null!");
    return frame_id_;
  }
  constexpr explicit(false) operator DataFrameID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data frame views by ID.
  friend constexpr auto operator==(DataFrameView lhs,
                                   DataFrameView rhs) noexcept -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.frame_id_ == rhs.frame_id_;
  }

  /// Get the time of the data frame.
  auto time() const -> float64_t {
    return storage().frame_time(frame_id_);
  }

  /// Get the number of data arrays in the frame.
  auto num_arrays() const -> size_t {
    return storage().frame_num_arrays(frame_id_);
  }

  /// Enumerate all data arrays in the frame.
  auto arrays() const {
    return storage().frame_arrays(frame_id_);
  }

  /// Find the data array with the given name.
  auto find_array(std::string_view name) const {
    return storage().find_array(frame_id_, name);
  }

  /// Create a new data array in the frame.
  template<class... Args>
  auto create_array(std::string_view name) const -> DataArrayView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().create_array(frame_id_, name);
  }

private:

  Storage* storage_ = nullptr;
  DataFrameID frame_id_{0};

}; // class DataFrameView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data series view.
template<data_storage Storage>
class DataSeriesView final {
public:

  /// Construct a null data series view.
  constexpr DataSeriesView() noexcept = default;

  /// Construct a data series view.
  constexpr explicit DataSeriesView(Storage& storage, DataSeriesID series_id)
      : storage_{&storage}, series_id_{series_id} {
    TIT_ASSERT(storage.check_series(series_id_), "Invalid series ID!");
  }

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the data series ID.
  /// @{
  constexpr auto id() const noexcept -> DataSeriesID {
    TIT_ASSERT(series_id_ != DataSeriesID{0}, "Series ID is null!");
    return series_id_;
  }
  constexpr explicit(false) operator DataSeriesID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data series views by ID.
  friend constexpr auto operator==(DataSeriesView lhs,
                                   DataSeriesView rhs) noexcept {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.series_id_ == rhs.series_id_;
  }

  /// Get the parameters of the data series.
  auto parameters() const -> std::string {
    return storage().series_parameters(series_id_);
  }

  /// Get the number of frames in the data series.
  auto num_frames() const -> size_t {
    return storage().series_num_frames(series_id_);
  }

  /// Enumerate all frames in the data series.
  auto frames() const {
    return storage().series_frames(series_id_);
  }

  /// Get the last frame in the series.
  auto last_frame() const {
    return storage().series_last_frame(series_id_);
  }

  /// Create a new frame in the data series.
  auto create_frame(float64_t time) const -> DataFrameView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().create_frame(series_id_, time);
  }

private:

  Storage* storage_ = nullptr;
  DataSeriesID series_id_{0};

}; // class DataSeriesView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data storage.
class DataStorage final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Open a data storage or create it if it does not exist.
  explicit DataStorage(const std::filesystem::path& path,
                       bool read_only = false);

  /// Path to the database file.
  auto path() const -> std::filesystem::path;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the maximum number of data series.
  auto max_series() const -> size_t;

  /// Set the maximum number of data series. If the number of series exceeds the
  /// maximum, the oldest series will be deleted.
  void set_max_series(size_t value);

  /// Number of data series in the storage.
  auto num_series() const -> size_t;

  /// Enumerate all data series.
  /// @{
  auto series_ids() const -> std::generator<DataSeriesID>;
  auto series(this auto& self) {
    return std::views::transform( //
        self.series_ids(),
        [&self](DataSeriesID id) { return DataSeriesView{self, id}; });
  }
  /// @}

  /// Get the last series.
  /// @{
  auto last_series_id() const -> DataSeriesID;
  auto last_series(this auto& self) {
    return DataSeriesView{self, self.last_series_id()};
  }
  /// @}

  /// Create a new data series.
  /// @{
  auto create_series_id(std::string_view parameters = "") -> DataSeriesID;
  auto create_series(std::string_view parameters = "")
      -> DataSeriesView<DataStorage> {
    return DataSeriesView{*this, create_series_id(parameters)};
  }
  /// @}

  /// Delete a data series.
  void delete_series(DataSeriesID series_id);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a data series with the given ID exists.
  auto check_series(DataSeriesID series_id) const -> bool;

  /// Get the parameters of a data series.
  auto series_parameters(DataSeriesID series_id) const -> std::string;

  /// Number of frames in the series.
  auto series_num_frames(DataSeriesID series_id) const -> size_t;

  /// Enumerate all frames in the data series.
  /// @{
  auto series_frame_ids(DataSeriesID series_id) const
      -> std::generator<DataFrameID>;
  auto series_frames(this auto& self, DataSeriesID series_id) {
    return std::views::transform(
        self.series_frame_ids(series_id),
        [&self](DataFrameID id) { return DataFrameView{self, id}; });
  }
  /// @}

  /// Get the last frame in the series.
  /// @{
  auto series_last_frame_id(DataSeriesID series_id) const -> DataFrameID;
  auto series_last_frame(this auto& self, DataSeriesID series_id) {
    return DataFrameView{self, self.series_last_frame_id(series_id)};
  }
  /// @}

  /// Create a new frame in the series.
  /// @{
  auto create_frame_id(DataSeriesID series_id, float64_t time) -> DataFrameID;
  auto create_frame(DataSeriesID series_id, float64_t time)
      -> DataFrameView<DataStorage> {
    return DataFrameView{*this, create_frame_id(series_id, time)};
  }
  /// @}

  /// Delete a frame.
  void delete_frame(DataFrameID frame_id);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a frame with the given ID exists.
  auto check_frame(DataFrameID frame_id) const -> bool;

  /// Get the time of a frame.
  auto frame_time(DataFrameID frame_id) const -> float64_t;

  /// Number of data arrays in the frame.
  auto frame_num_arrays(DataFrameID frame_id) const -> size_t;

  /// Enumerate all data arrays in the frame.
  /// @{
  auto frame_array_ids(DataFrameID frame_id) const
      -> std::generator<DataArrayID>;
  auto frame_arrays(this auto& self, DataFrameID frame_id) {
    return std::views::transform(
        self.frame_array_ids(frame_id),
        [&self](auto id) { return DataArrayView{self, id}; });
  }
  /// @}

  /// Find the data array with the given name.
  /// @{
  auto find_array_id(DataFrameID frame_id, std::string_view name) const
      -> std::optional<DataArrayID>;
  auto find_array(this auto& self,
                  DataFrameID frame_id,
                  std::string_view name) {
    return self.find_array_id(frame_id, name).transform([&self](auto id) {
      return DataArrayView{self, id};
    });
  }
  /// @}

  /// Create a new data array in the frame.
  /// @{
  auto create_array_id(DataFrameID frame_id, std::string_view name)
      -> DataArrayID;
  template<class... Args>
  auto create_array(DataFrameID frame_id, std::string_view name)
      -> DataArrayView<DataStorage> {
    return DataArrayView{*this, create_array_id(frame_id, name)};
  }
  /// @}

  /// Delete a data array.
  void delete_array(DataArrayID array_id);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a data array with the given ID exists.
  auto check_array(DataArrayID array_id) const -> bool;

  /// Get the name of a data array.
  auto array_name(DataArrayID array_id) const -> std::string;

  /// Get the data type of the data array.
  auto array_type(DataArrayID array_id) const -> DataType;

  /// Get the number of elements in the data array.
  auto array_size(DataArrayID array_id) const -> size_t;

  /// Open an output stream to write the data of a data array.
  auto array_open_write(DataArrayID array_id, DataType type, size_t size)
      -> OutputStreamPtr<byte_t>;

  /// Write data to a data array.
  template<known_type_of Val>
  void array_write(DataArrayID array_id, std::span<const Val> data) {
    make_stream_serializer<Val>(
        array_open_write(array_id, type_of<Val>, data.size()))
        ->write(data);
  }

  /// Open an input stream to read the data of a data array.
  /// @{
  auto array_open_read(DataArrayID array_id) const -> InputStreamPtr<byte_t>;
  template<known_type_of Val>
  auto array_open_read(DataArrayID array_id) const -> InputStreamPtr<Val> {
    TIT_ASSERT(array_type(array_id) == type_of<Val>, "Type mismatch!");
    return make_stream_deserializer<Val>(array_open_read(array_id));
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable sqlite::Database db_;

}; // class Database

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
