/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/numbers/strict.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"

#include "tit/core/utils.hpp"
#include "tit/data/sqlite.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
struct DataSeriesTag;
struct DataTimeStepTag;
struct DataSetTag;
struct DataArrayTag;
} // namespace impl

/// Data series ID type.
using DataSeriesID = Strict<sqlite::RowID, impl::DataSeriesTag>;

/// Data time step ID type.
using DataTimeStepID = Strict<sqlite::RowID, impl::DataTimeStepTag>;

/// Dataset ID type.
using DataSetID = Strict<sqlite::RowID, impl::DataSetTag>;

/// Data array ID type.
using DataArrayID = Strict<sqlite::RowID, impl::DataArrayTag>;

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
    TIT_ASSERT(array_id_.get() != 0, "Array ID is null!");
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

  /// Get the data type of the data array.
  auto type() const -> DataType {
    return storage().array_type(array_id_);
  }

  /// Get the size of a data array (in elements).
  auto size() const -> size_t {
    return storage().array_size(array_id_);
  }

  /// Open an output stream to write the data.
  /// @{
  auto open_write() const -> OutputStreamPtr<byte_t>
    requires (!std::is_const_v<Storage>)
  {
    return storage().array_data_open_write(array_id_);
  }
  template<known_type_of Val>
  auto open_write() const -> OutputStreamPtr<Val>
    requires (!std::is_const_v<Storage>)
  {
    return storage().template array_data_open_write<Val>(array_id_);
  }
  /// @}

  /// Open an input stream to read the data.
  /// @{
  auto open_read() const -> InputStreamPtr<byte_t> {
    return storage().array_data_open_read(array_id_);
  }
  template<known_type_of Val>
  auto open_read() const -> InputStreamPtr<Val> {
    return storage().template array_data_open_read<Val>(array_id_);
  }
  /// @}

private:

  Storage* storage_ = nullptr;
  DataArrayID array_id_{0};

}; // class DataArrayView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dataset view.
template<data_storage Storage>
class DataSetView final {
public:

  /// Construct a null data set view.
  constexpr DataSetView() noexcept = default;

  /// Construct a dataset view.
  constexpr explicit DataSetView(Storage& storage, DataSetID dataset_id)
      : storage_{&storage}, dataset_id_{dataset_id} {
    TIT_ASSERT(storage.check_dataset(dataset_id_), "Invalid dataset ID!");
  }

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the dataset ID.
  /// @{
  constexpr auto id() const noexcept -> DataSetID {
    TIT_ASSERT(dataset_id_.get() != 0, "Data set ID is null!");
    return dataset_id_;
  }
  constexpr explicit(false) operator DataSetID() const noexcept {
    return id();
  }
  /// @}

  /// Compare dataset views by ID.
  friend constexpr auto operator==(DataSetView lhs, DataSetView rhs) noexcept
      -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.dataset_id_ == rhs.dataset_id_;
  }

  /// Get the number of data arrays in the dataset.
  auto num_arrays() const -> size_t {
    return storage().dataset_num_arrays(dataset_id_);
  }

  /// Enumerate all data arrays in the dataset.
  auto arrays() const {
    return storage().dataset_arrays(dataset_id_);
  }

  /// Find the data array with the given name.
  auto find_array(std::string_view name) const {
    return storage().find_array(dataset_id_, name);
  }

  /// Create a new data array in the dataset.
  template<class... Args>
  auto create_array(std::string_view name, Args&&... args) const
      -> DataArrayView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().create_array(dataset_id_,
                                  name,
                                  std::forward<Args>(args)...);
  }

private:

  Storage* storage_ = nullptr;
  DataSetID dataset_id_{0};

}; // class DataSetView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data time step view.
template<data_storage Storage>
class DataTimeStepView final {
public:

  /// Construct a null data time step view.
  constexpr DataTimeStepView() noexcept = default;

  /// Construct a data time step view.
  constexpr explicit DataTimeStepView(Storage& storage,
                                      DataTimeStepID time_step_id)
      : storage_{&storage}, time_step_id_{time_step_id} {
    TIT_ASSERT(storage.check_time_step(time_step_id_), "Invalid time step ID!");
  }

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the data time step ID.
  /// @{
  constexpr auto id() const noexcept -> DataTimeStepID {
    TIT_ASSERT(time_step_id_.get() != 0, "Time step ID is null!");
    return time_step_id_;
  }
  constexpr explicit(false) operator DataTimeStepID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data time step views by ID.
  friend constexpr auto operator==(DataTimeStepView lhs,
                                   DataTimeStepView rhs) noexcept -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.time_step_id_ == rhs.time_step_id_;
  }

  /// Get the time of the data time step.
  auto time() const -> real_t {
    return storage().time_step_time(time_step_id_);
  }

  /// Get the uniform dataset of the data time step.
  auto uniforms() const -> DataSetView<Storage> {
    return storage().time_step_uniforms(time_step_id_);
  }

  /// Get the varying dataset of the data time step.
  auto varyings() const -> DataSetView<Storage> {
    return storage().time_step_varyings(time_step_id_);
  }

private:

  Storage* storage_ = nullptr;
  DataTimeStepID time_step_id_{0};

}; // class DataTimeStepView

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
    TIT_ASSERT(series_id_.get() != 0, "Series ID is null!");
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

  /// Get the number of time steps in the data series.
  auto num_time_steps() const -> size_t {
    return storage().series_num_time_steps(series_id_);
  }

  /// Enumerate all time steps in the data series.
  auto time_steps() const {
    return storage().series_time_steps(series_id_);
  }

  /// Get the last time step in the series.
  auto last_time_step() const {
    return storage().series_last_time_step(series_id_);
  }

  /// Create a new time step in the data series.
  auto create_time_step(real_t time) const -> DataTimeStepView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return storage().create_time_step(series_id_, time);
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
  explicit DataStorage(const std::filesystem::path& path);

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
  auto series_ids() const -> InputStreamPtr<DataSeriesID>;
  auto series(this auto& self) {
    return transform_stream(self.series_ids(), [&self](DataSeriesID id) {
      return DataSeriesView{self, id};
    });
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

  /// Number of time steps in the series.
  auto series_num_time_steps(DataSeriesID series_id) const -> size_t;

  /// Enumerate all time steps in the series.
  /// @{
  auto series_time_step_ids(DataSeriesID series_id) const
      -> InputStreamPtr<DataTimeStepID>;
  auto series_time_steps(this auto& self, DataSeriesID series_id) {
    return transform_stream(
        self.series_time_step_ids(series_id),
        [&self](DataTimeStepID id) { return DataTimeStepView{self, id}; });
  }
  /// @}

  /// Get the last time step in the series.
  /// @{
  auto series_last_time_step_id(DataSeriesID series_id) const -> DataTimeStepID;
  auto series_last_time_step(this auto& self, DataSeriesID series_id) {
    return DataTimeStepView{self, self.series_last_time_step_id(series_id)};
  }
  /// @}

  /// Create a new time step in the series.
  /// @{
  auto create_time_step_id(DataSeriesID series_id, real_t time)
      -> DataTimeStepID;
  auto create_time_step(DataSeriesID series_id, real_t time)
      -> DataTimeStepView<DataStorage> {
    return DataTimeStepView{*this, create_time_step_id(series_id, time)};
  }
  /// @}

  /// Delete a time step.
  void delete_time_step(DataTimeStepID time_step_id);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a time step with the given ID exists.
  auto check_time_step(DataTimeStepID time_step_id) const -> bool;

  /// Get the time of a time step.
  auto time_step_time(DataTimeStepID time_step_id) const -> real_t;

  /// Get the uniform dataset of a time step.
  /// @{
  auto time_step_uniforms_id(DataTimeStepID time_step_id) const -> DataSetID;
  auto time_step_uniforms(this auto& self, DataTimeStepID time_step_id) {
    return DataSetView{self, self.time_step_uniforms_id(time_step_id)};
  }
  /// @}

  /// Get the varying dataset of a time step.
  /// @{
  auto time_step_varyings_id(DataTimeStepID time_step_id) const -> DataSetID;
  auto time_step_varyings(this auto& self, DataTimeStepID time_step_id) {
    return DataSetView{self, self.time_step_varyings_id(time_step_id)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a dataset with the given ID exists.
  auto check_dataset(DataSetID dataset_id) const -> bool;

  /// Number of data arrays in the dataset.
  auto dataset_num_arrays(DataSetID dataset_id) const -> size_t;

  /// Enumerate all data arrays in the dataset.
  /// @{
  auto dataset_array_ids(DataSetID dataset_id) const
      -> InputStreamPtr<std::pair<std::string, DataArrayID>>;
  auto dataset_arrays(this auto& self, DataSetID dataset_id) {
    return transform_stream( //
        self.dataset_array_ids(dataset_id),
        [&self](auto name_and_id) {
          auto [name, id] = std::move(name_and_id);
          return std::pair{std::move(name), DataArrayView{self, id}};
        });
  }
  /// @}

  /// Find the data array with the given name.
  /// @{
  auto find_array_id(DataSetID dataset_id, std::string_view name) const
      -> std::optional<DataArrayID>;
  auto find_array(this auto& self,
                  DataSetID dataset_id,
                  std::string_view name) {
    return self.find_array_id(dataset_id, name).transform([&self](auto id) {
      return DataArrayView{self, id};
    });
  }
  /// @}

  /// Create a new data array in the dataset.
  /// @{
  auto create_array_id(DataSetID dataset_id,
                       std::string_view name,
                       DataType type) -> DataArrayID;
  auto create_array_id(DataSetID dataset_id,
                       std::string_view name,
                       DataType type,
                       std::span<const byte_t> data) -> DataArrayID;
  template<std::ranges::input_range Vals>
    requires known_type_of<std::ranges::range_value_t<Vals>>
  auto create_array_id(DataSetID dataset_id, std::string_view name, Vals&& vals)
      -> DataArrayID {
    TIT_ASSUME_UNIVERSAL(Vals, vals);
    using Val = std::ranges::range_value_t<Vals>;
    const auto array_id = create_array_id(dataset_id, name, type_of<Val>);
    array_data_open_write<Val>(array_id)->write(vals);
    return array_id;
  }
  template<class... Args>
  auto create_array(DataSetID dataset_id, std::string_view name, Args&&... args)
      -> DataArrayView<DataStorage> {
    return DataArrayView{
        *this,
        create_array_id(dataset_id, name, std::forward<Args>(args)...)};
  }
  /// @}

  /// Delete a data array.
  void delete_array(DataArrayID array_id);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a data array with the given ID exists.
  auto check_array(DataArrayID array_id) const -> bool;

  /// Get the data type of the data array.
  auto array_type(DataArrayID array_id) const -> DataType;

  /// Get the number of elements in the data array.
  auto array_size(DataArrayID array_id) const -> size_t;

  /// Open an output stream to write the data of a data array.
  /// @{
  auto array_data_open_write(DataArrayID array_id) -> OutputStreamPtr<byte_t>;
  template<known_type_of Val>
  auto array_data_open_write(DataArrayID array_id) -> OutputStreamPtr<Val> {
    TIT_ASSERT(array_type(array_id) == type_of<Val>, "Type mismatch!");
    return make_stream_serializer<Val>(array_data_open_write(array_id));
  }
  /// @}

  /// Open an input stream to read the data of a data array.
  /// @{
  auto array_data_open_read(DataArrayID array_id) const
      -> InputStreamPtr<byte_t>;
  template<known_type_of Val>
  auto array_data_open_read(DataArrayID array_id) const -> InputStreamPtr<Val> {
    TIT_ASSERT(array_type(array_id) == type_of<Val>, "Type mismatch!");
    return make_stream_deserializer<Val>(array_data_open_read(array_id));
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Create a new dataset.
  auto create_set_() -> DataSetID;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  mutable sqlite::Database db_;

}; // class Database

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
