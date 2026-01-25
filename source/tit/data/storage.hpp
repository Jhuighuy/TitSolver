/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <generator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/param_spec.hpp"
#include "tit/data/sqlite.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data series ID type.
enum class DataSeriesID : sqlite::RowID {};

/// Parameter ID type.
enum class DataParamID : sqlite::RowID {};

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
  /// @{
  constexpr explicit DataArrayView(Storage& storage, DataArrayID array_id)
      : storage_{&storage}, array_id_{array_id} {
    TIT_ASSERT(storage.check_array(array_id_), "Invalid data array ID!");
  }
  template<data_storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) DataArrayView(DataArrayView<Other> other)
      : storage_{&other.storage()}, array_id_{other.id()} {}
  /// @}

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

  /// Write data to the data array.
  /// @{
  void write(DataType type, std::span<const std::byte> data) const {
    storage().array_write(array_id_, type, data);
  }
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void write(Range&& data) const {
    storage().array_write(array_id_, std::forward<Range>(data));
  }
  /// @}

  /// Read data from the data array.
  /// @{
  void read(std::span<std::byte> data) const {
    storage().array_read(array_id_, data);
  }
  auto read() const -> std::vector<std::byte> {
    return storage().array_read(array_id_);
  }
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void read(Range&& data) const {
    storage().array_read(array_id_, std::forward<Range>(data));
  }
  template<known_type_of Val>
  auto read() const -> std::vector<Val> {
    return storage().template array_read<Val>(array_id_);
  }
  /// @}

private:

  Storage* storage_ = nullptr;
  DataArrayID array_id_{0};

}; // class DataArrayView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Parameter view.
template<data_storage Storage>
class DataParamView final {
public:

  /// Construct a null parameter view.
  constexpr DataParamView() noexcept = default;

  /// Construct a parameter view.
  /// @{
  constexpr explicit DataParamView(Storage& storage, DataParamID param_id)
      : storage_{&storage}, param_id_{param_id} {
    TIT_ASSERT(storage.check_param(param_id_), "Invalid parameter ID!");
  }
  template<data_storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) DataParamView(DataParamView<Other> other)
      : storage_{&other.storage()}, param_id_{other.id()} {}
  /// @}

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the parameter ID.
  /// @{
  constexpr auto id() const noexcept -> DataParamID {
    TIT_ASSERT(param_id_ != DataParamID{0}, "Parameter ID is null!");
    return param_id_;
  }
  constexpr explicit(false) operator DataParamID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data parameter views by ID.
  friend constexpr auto operator==(DataParamView lhs,
                                   DataParamView rhs) noexcept -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.param_id_ == rhs.param_id_;
  }

  /// Get the specification of the parameter.
  auto spec() const -> ParamSpecPtr {
    return storage().param_spec(param_id_);
  }

  /// Get the value of the parameter.
  auto value() const -> std::string {
    return storage().param_value(param_id_);
  }

  /// Set the value of the parameter.
  void set_value(std::string_view value) {
    storage().param_set_value(param_id_, value);
  }

  /// Get the parent parameter ID.
  auto parent_id() const -> DataParamID {
    return storage().param_parent_id(param_id_);
  }

  /// Get the number of child parameters.
  auto num_children() const -> size_t {
    return storage().param_num_children(param_id_);
  }

  /// Enumerate all child parameters.
  auto children() const {
    return storage().param_children(param_id_);
  }

private:

  Storage* storage_ = nullptr;
  DataParamID param_id_{0};

}; // class ParameterView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data frame view.
template<data_storage Storage>
class DataFrameView final {
public:

  /// Construct a null data frame view.
  constexpr DataFrameView() noexcept = default;

  /// Construct a data frame view.
  /// @{
  constexpr explicit DataFrameView(Storage& storage, DataFrameID frame_id)
      : storage_{&storage}, frame_id_{frame_id} {
    TIT_ASSERT(storage.check_frame(frame_id_), "Invalid frame ID!");
  }
  template<data_storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) DataFrameView(DataFrameView<Other> other)
      : storage_{&other.storage()}, frame_id_{other.id()} {}
  /// @}

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
    return storage().frame_find_array(frame_id_, name);
  }

  /// Create a new data array in the frame.
  auto create_array(std::string_view name) const -> DataArrayView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().frame_create_array(frame_id_, name);
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
  /// @{
  constexpr explicit DataSeriesView(Storage& storage, DataSeriesID series_id)
      : storage_{&storage}, series_id_{series_id} {
    TIT_ASSERT(storage.check_series(series_id_), "Invalid series ID!");
  }
  template<data_storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) DataSeriesView(DataSeriesView<Other> other)
      : storage_{&other.storage()}, series_id_{other.id()} {}
  /// @}

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

  /// Get the name of the data series.
  auto name() const -> std::string {
    return storage().series_name(series_id_);
  }

  /// Get the number of parameters in the data series.
  auto num_params() const -> size_t {
    return storage().series_num_params(series_id_);
  }

  /// Enumerate all parameters in the data series.
  auto params() const {
    return storage().series_params(series_id_);
  }

  /// Create a new parameter in the data series.
  auto create_param(const ParamSpec& spec,
                    DataParamID parent_id = DataParamID{0},
                    std::string_view value = "") const
      -> DataParamView<DataStorage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().series_create_param(series_id_, spec, parent_id, value);
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
    return storage().series_create_frame(series_id_, time);
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
    return self.series_ids() | //
           std::views::transform(
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
  auto create_series_id(std::string_view name = "") -> DataSeriesID;
  auto create_series(std::string_view name = "")
      -> DataSeriesView<DataStorage> {
    return DataSeriesView{*this, create_series_id(name)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a data series.
  void delete_series(DataSeriesID series_id);

  /// Check if a data series with the given ID exists.
  auto check_series(DataSeriesID series_id) const -> bool;

  /// Get the name of a data series.
  auto series_name(DataSeriesID series_id) const -> std::string;

  /// Number of parameters in the series.
  auto series_num_params(DataSeriesID series_id) const -> size_t;

  /// Enumerate all parameters in the series.
  /// @{
  auto series_param_ids(DataSeriesID series_id) const
      -> std::generator<DataParamID>;
  auto series_params(this auto& self, DataSeriesID series_id) {
    return self.series_param_ids(series_id) |
           std::views::transform(
               [&self](DataParamID id) { return DataParamView{self, id}; });
  }
  /// @}

  /// Create a new parameter in the series.
  /// @{
  auto series_create_param_id(DataSeriesID series_id,
                              const ParamSpec& spec,
                              DataParamID parent_id = DataParamID{0},
                              std::string_view value = "") -> DataParamID;
  auto series_create_param(DataSeriesID series_id,
                           const ParamSpec& spec,
                           DataParamID parent_id = DataParamID{0},
                           std::string_view value = "")
      -> DataParamView<DataStorage> {
    return DataParamView{
        *this,
        series_create_param_id(series_id, spec, parent_id, value)};
  }
  /// @}

  /// Number of frames in the series.
  auto series_num_frames(DataSeriesID series_id) const -> size_t;

  /// Enumerate all frames in the data series.
  /// @{
  auto series_frame_ids(DataSeriesID series_id) const
      -> std::generator<DataFrameID>;
  auto series_frames(this auto& self, DataSeriesID series_id) {
    return self.series_frame_ids(series_id) |
           std::views::transform(
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
  auto series_create_frame_id(DataSeriesID series_id, float64_t time)
      -> DataFrameID;
  auto series_create_frame(DataSeriesID series_id, float64_t time)
      -> DataFrameView<DataStorage> {
    return DataFrameView{*this, series_create_frame_id(series_id, time)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a parameter.
  void delete_param(DataParamID param_id);

  /// Check if a parameter with the given ID exists.
  auto check_param(DataParamID param_id) const -> bool;

  /// Get the specification of the parameter.
  auto param_spec(DataParamID param_id) const -> ParamSpecPtr;

  /// Get the value of a parameter.
  auto param_value(DataParamID param_id) const -> std::string;

  /// Set the value of a parameter.
  void param_set_value(DataParamID param_id, std::string_view value);

  /// Get the parent parameter.
  auto param_parent_id(DataParamID param_id) const -> DataParamID;

  /// Get the number of child parameters.
  auto param_num_children(DataParamID param_id) const -> size_t;

  /// Enumerate all child parameters.
  /// @{
  auto param_child_ids(DataParamID param_id) const
      -> std::generator<DataParamID>;
  auto param_children(this auto& self, DataParamID param_id) {
    return self.param_child_ids(param_id) |
           std::views::transform(
               [&self](auto id) { return DataParamView{self, id}; });
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a frame.
  void delete_frame(DataFrameID frame_id);

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
    return self.frame_array_ids(frame_id) |
           std::views::transform(
               [&self](auto id) { return DataArrayView{self, id}; });
  }
  /// @}

  /// Find the data array with the given name.
  /// @{
  auto frame_find_array_id(DataFrameID frame_id, std::string_view name) const
      -> std::optional<DataArrayID>;
  auto frame_find_array(this auto& self,
                        DataFrameID frame_id,
                        std::string_view name) {
    return self.frame_find_array_id(frame_id, name).transform([&self](auto id) {
      return DataArrayView{self, id};
    });
  }
  /// @}

  /// Create a new data array in the frame.
  /// @{
  auto frame_create_array_id(DataFrameID frame_id, std::string_view name)
      -> DataArrayID;
  auto frame_create_array(DataFrameID frame_id, std::string_view name)
      -> DataArrayView<DataStorage> {
    return DataArrayView{*this, frame_create_array_id(frame_id, name)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a data array.
  void delete_array(DataArrayID array_id);

  /// Check if a data array with the given ID exists.
  auto check_array(DataArrayID array_id) const -> bool;

  /// Get the name of a data array.
  auto array_name(DataArrayID array_id) const -> std::string;

  /// Get the data type of the data array.
  auto array_type(DataArrayID array_id) const -> DataType;

  /// Get the number of elements in the data array.
  auto array_size(DataArrayID array_id) const -> size_t;

  /// Write data to a data array.
  /// @{
  void array_write(DataArrayID array_id,
                   DataType type,
                   std::span<const std::byte> data);
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void array_write(DataArrayID array_id, Range&& data) {
    using Val = std::ranges::range_value_t<Range>;
    make_stream_serializer<Val>(
        array_open_write_(array_id, type_of<Val>, data.size()))
        ->write(std::forward<Range>(data));
  }
  /// @}

  /// Read data from a data array.
  /// @{
  void array_read(DataArrayID array_id, std::span<std::byte> data) const;
  auto array_read(DataArrayID array_id) const -> std::vector<std::byte>;
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void array_read(DataArrayID array_id, Range&& data) const {
    using Val = std::ranges::range_value_t<Range>;
    TIT_ASSERT(array_type(array_id) == type_of<Val>, "Type mismatch!");
    TIT_ASSERT(data.size() == array_size(array_id), "Data size mismatch!");
    make_stream_deserializer<Val>(array_open_read_(array_id))
        ->read(std::forward<Range>(data));
  }
  template<known_type_of Val>
  auto array_read(DataArrayID array_id) const -> std::vector<Val> {
    std::vector<Val> result(array_size(array_id));
    array_read(array_id, result);
    return result;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Open an output stream to write the data of a data array.
  auto array_open_write_(DataArrayID array_id, DataType type, size_t size)
      -> OutputStreamPtr<std::byte>;

  // Open an input stream to read the data of a data array.
  auto array_open_read_(DataArrayID array_id) const
      -> InputStreamPtr<std::byte>;

  mutable sqlite::Database db_;

}; // class Database

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
