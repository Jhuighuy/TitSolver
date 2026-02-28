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
#include "tit/data/sqlite.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data series ID type.
enum class SeriesID : sqlite::RowID {};

/// Data frame ID type.
enum class FrameID : sqlite::RowID {};

/// Data array ID type.
enum class ArrayID : sqlite::RowID {};

/// Data storage type.
template<class S>
concept storage = std::same_as<std::remove_const_t<S>, class Storage>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Array view.
template<storage Storage>
class ArrayView final {
public:

  /// Construct a null data array view.
  constexpr ArrayView() noexcept = default;

  /// Construct a data array view.
  /// @{
  constexpr explicit ArrayView(Storage& storage, ArrayID array_id)
      : storage_{&storage}, array_id_{array_id} {
    TIT_ASSERT(storage.check_array(array_id_), "Invalid data array ID!");
  }
  template<storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) ArrayView(ArrayView<Other> other)
      : storage_{&other.storage()}, array_id_{other.id()} {}
  /// @}

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the data array ID.
  /// @{
  constexpr auto id() const noexcept -> ArrayID {
    TIT_ASSERT(array_id_ != ArrayID{0}, "Array ID is null!");
    return array_id_;
  }
  constexpr explicit(false) operator ArrayID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data array views by ID.
  friend constexpr auto operator==(ArrayView lhs, ArrayView rhs) noexcept
      -> bool {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.array_id_ == rhs.array_id_;
  }

  /// Get the name of the data array.
  auto name() const -> std::string {
    return storage().array_name(array_id_);
  }

  /// Get the data type of the data array.
  auto type() const -> Type {
    return storage().array_type(array_id_);
  }

  /// Get the size of a data array (in elements).
  auto size() const -> size_t {
    return storage().array_size(array_id_);
  }

  /// Write data to the data array.
  /// @{
  void write(Type type, std::span<const std::byte> data) const {
    storage().array_write(array_id_, type, data);
  }
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void write(Range&& data) const {
    storage().array_write(array_id_, data);
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
    storage().array_read(array_id_, data);
  }
  template<known_type_of Val>
  auto read() const -> std::vector<Val> {
    return storage().template array_read<Val>(array_id_);
  }
  /// @}

private:

  Storage* storage_ = nullptr;
  ArrayID array_id_{0};

}; // class ArrayView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Frame view.
template<storage Storage>
class FrameView final {
public:

  /// Construct a null data frame view.
  constexpr FrameView() noexcept = default;

  /// Construct a data frame view.
  /// @{
  constexpr explicit FrameView(Storage& storage, FrameID frame_id)
      : storage_{&storage}, frame_id_{frame_id} {
    TIT_ASSERT(storage.check_frame(frame_id_), "Invalid frame ID!");
  }
  template<storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) FrameView(FrameView<Other> other)
      : storage_{&other.storage()}, frame_id_{other.id()} {}
  /// @}

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the frame ID.
  /// @{
  constexpr auto id() const noexcept -> FrameID {
    TIT_ASSERT(frame_id_ != FrameID{0}, "Frame ID is null!");
    return frame_id_;
  }
  constexpr explicit(false) operator FrameID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data frame views by ID.
  friend constexpr auto operator==(FrameView lhs, FrameView rhs) noexcept
      -> bool {
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
  auto create_array(std::string_view name) const -> ArrayView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().frame_create_array(frame_id_, name);
  }

private:

  Storage* storage_ = nullptr;
  FrameID frame_id_{0};

}; // class DataFrameView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Series view.
template<storage Storage>
class SeriesView final {
public:

  /// Construct a null data series view.
  constexpr SeriesView() noexcept = default;

  /// Construct a data series view.
  /// @{
  constexpr explicit SeriesView(Storage& storage, SeriesID series_id)
      : storage_{&storage}, series_id_{series_id} {
    TIT_ASSERT(storage.check_series(series_id_), "Invalid series ID!");
  }
  template<storage Other>
    requires (!std::same_as<Other, Storage> &&
              std::convertible_to<Other&, Storage&>)
  constexpr explicit(false) SeriesView(SeriesView<Other> other)
      : storage_{&other.storage()}, series_id_{other.id()} {}
  /// @}

  /// Get the data storage.
  constexpr auto storage() const noexcept -> Storage& {
    TIT_ASSERT(storage_ != nullptr, "Storage is null!");
    return *storage_;
  }

  /// Get the data series ID.
  /// @{
  constexpr auto id() const noexcept -> SeriesID {
    TIT_ASSERT(series_id_ != SeriesID{0}, "Series ID is null!");
    return series_id_;
  }
  constexpr explicit(false) operator SeriesID() const noexcept {
    return id();
  }
  /// @}

  /// Compare data series views by ID.
  friend constexpr auto operator==(SeriesView lhs, SeriesView rhs) noexcept {
    TIT_ASSERT(&lhs.storage() == &rhs.storage(), "Incompatible data storages!");
    return lhs.series_id_ == rhs.series_id_;
  }

  /// Get the name of the data series.
  auto name() const -> std::string {
    return storage().series_name(series_id_);
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
  auto create_frame(float64_t time) const -> FrameView<Storage>
    requires (!std::is_const_v<Storage>)
  {
    return storage().series_create_frame(series_id_, time);
  }

private:

  Storage* storage_ = nullptr;
  SeriesID series_id_{0};

}; // class SeriesView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Storage.
class Storage final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Open a data storage or create it if it does not exist.
  explicit Storage(const std::filesystem::path& path, bool read_only = false);

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
  auto series_ids() const -> std::generator<SeriesID>;
  auto series(this auto& self) {
    return self.series_ids() | //
           std::views::transform(
               [&self](SeriesID id) { return SeriesView{self, id}; });
  }
  /// @}

  /// Get the last series.
  /// @{
  auto last_series_id() const -> SeriesID;
  auto last_series(this auto& self) {
    return SeriesView{self, self.last_series_id()};
  }
  /// @}

  /// Create a new data series.
  /// @{
  auto create_series_id(std::string_view name = "") -> SeriesID;
  auto create_series(std::string_view name = "") -> SeriesView<Storage> {
    return SeriesView{*this, create_series_id(name)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a data series.
  void delete_series(SeriesID series_id);

  /// Check if a data series with the given ID exists.
  auto check_series(SeriesID series_id) const -> bool;

  /// Get the name of a data series.
  auto series_name(SeriesID series_id) const -> std::string;

  /// Number of frames in the series.
  auto series_num_frames(SeriesID series_id) const -> size_t;

  /// Enumerate all frames in the data series.
  /// @{
  auto series_frame_ids(SeriesID series_id) const -> std::generator<FrameID>;
  auto series_frames(this auto& self, SeriesID series_id) {
    return self.series_frame_ids(series_id) |
           std::views::transform(
               [&self](FrameID id) { return FrameView{self, id}; });
  }
  /// @}

  /// Get the last frame in the series.
  /// @{
  auto series_last_frame_id(SeriesID series_id) const -> FrameID;
  auto series_last_frame(this auto& self, SeriesID series_id) {
    return FrameView{self, self.series_last_frame_id(series_id)};
  }
  /// @}

  /// Create a new frame in the series.
  /// @{
  auto series_create_frame_id(SeriesID series_id, float64_t time) -> FrameID;
  auto series_create_frame(SeriesID series_id, float64_t time)
      -> FrameView<Storage> {
    return FrameView{*this, series_create_frame_id(series_id, time)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a frame.
  void delete_frame(FrameID frame_id);

  /// Check if a frame with the given ID exists.
  auto check_frame(FrameID frame_id) const -> bool;

  /// Get the time of a frame.
  auto frame_time(FrameID frame_id) const -> float64_t;

  /// Number of data arrays in the frame.
  auto frame_num_arrays(FrameID frame_id) const -> size_t;

  /// Enumerate all data arrays in the frame.
  /// @{
  auto frame_array_ids(FrameID frame_id) const -> std::generator<ArrayID>;
  auto frame_arrays(this auto& self, FrameID frame_id) {
    return self.frame_array_ids(frame_id) |
           std::views::transform(
               [&self](auto id) { return ArrayView{self, id}; });
  }
  /// @}

  /// Find the data array with the given name.
  /// @{
  auto frame_find_array_id(FrameID frame_id, std::string_view name) const
      -> std::optional<ArrayID>;
  auto frame_find_array(this auto& self,
                        FrameID frame_id,
                        std::string_view name) {
    return self.frame_find_array_id(frame_id, name).transform([&self](auto id) {
      return ArrayView{self, id};
    });
  }
  /// @}

  /// Create a new data array in the frame.
  /// @{
  auto frame_create_array_id(FrameID frame_id, std::string_view name)
      -> ArrayID;
  auto frame_create_array(FrameID frame_id, std::string_view name)
      -> ArrayView<Storage> {
    return ArrayView{*this, frame_create_array_id(frame_id, name)};
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Delete a data array.
  void delete_array(ArrayID array_id);

  /// Check if a data array with the given ID exists.
  auto check_array(ArrayID array_id) const -> bool;

  /// Get the name of a data array.
  auto array_name(ArrayID array_id) const -> std::string;

  /// Get the data type of the data array.
  auto array_type(ArrayID array_id) const -> Type;

  /// Get the number of elements in the data array.
  auto array_size(ArrayID array_id) const -> size_t;

  /// Write data to a data array.
  /// @{
  void array_write(ArrayID array_id,
                   Type type,
                   std::span<const std::byte> data);
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void array_write(ArrayID array_id, Range&& data) {
    using Val = std::ranges::range_value_t<Range>;
    make_stream_serializer<Val>(
        array_open_write_(array_id, type_of<Val>, data.size()))
        ->write(data);
  }
  /// @}

  /// Read data from a data array.
  /// @{
  void array_read(ArrayID array_id, std::span<std::byte> data) const;
  auto array_read(ArrayID array_id) const -> std::vector<std::byte>;
  template<std::ranges::sized_range Range>
    requires std::ranges::contiguous_range<Range> &&
             known_type_of<std::ranges::range_value_t<Range>>
  void array_read(ArrayID array_id, Range&& data) const {
    using Val = std::ranges::range_value_t<Range>;
    TIT_ASSERT(array_type(array_id) == type_of<Val>, "Type mismatch!");
    TIT_ASSERT(data.size() == array_size(array_id), "Data size mismatch!");
    make_stream_deserializer<Val>(array_open_read_(array_id))->read(data);
  }
  template<known_type_of Val>
  auto array_read(ArrayID array_id) const -> std::vector<Val> {
    std::vector<Val> result(array_size(array_id));
    array_read(array_id, result);
    return result;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Open an output stream to write the data of a data array.
  auto array_open_write_(ArrayID array_id, Type type, size_t size)
      -> OutputStreamPtr<std::byte>;

  // Open an input stream to read the data of a data array.
  auto array_open_read_(ArrayID array_id) const -> InputStreamPtr<std::byte>;

  mutable sqlite::Database db_;

}; // class Storage

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
