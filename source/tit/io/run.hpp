/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/io/type.hpp"

namespace tit::io {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Current on-disk run-format version.
inline constexpr std::uint32_t run_format_version = 1;

/// Descriptive metadata shared by every frame in a run.
class RunMetadata final {
public:

  RunMetadata(std::string name, std::size_t dimension)
      : name_{std::move(name)}, dimension_{dimension} {}

  auto name() const noexcept -> const std::string& {
    return name_;
  }

  auto dimension() const noexcept -> std::size_t {
    return dimension_;
  }

  auto operator==(const RunMetadata&) const -> bool = default;

private:

  std::string name_;
  std::size_t dimension_;

}; // class RunMetadata

/// Description of a committed frame.
class FrameDescriptor final {
public:

  constexpr FrameDescriptor(std::uint64_t step, double time) noexcept
      : step_{step}, time_{time} {}

  constexpr auto step() const noexcept -> std::uint64_t {
    return step_;
  }

  constexpr auto time() const noexcept -> double {
    return time_;
  }

  auto operator==(const FrameDescriptor&) const -> bool = default;

private:

  std::uint64_t step_;
  double time_;

}; // class FrameDescriptor

/// Description of a field in a frame.
class FieldDescriptor final {
public:

  FieldDescriptor(std::string name, Type type, std::size_t size)
      : name_{std::move(name)}, type_{type}, size_{size} {}

  auto name() const noexcept -> const std::string& {
    return name_;
  }

  constexpr auto type() const noexcept -> Type {
    return type_;
  }

  constexpr auto size() const noexcept -> std::size_t {
    return size_;
  }

  auto operator==(const FieldDescriptor&) const -> bool = default;

private:

  std::string name_;
  Type type_;
  std::size_t size_;

}; // class FieldDescriptor

/// A field descriptor and its contiguous typed bytes.
class FieldData final {
public:

  FieldData(FieldDescriptor descriptor, std::vector<std::byte> data)
      : descriptor_{std::move(descriptor)}, data_{std::move(data)} {}

  auto descriptor() const noexcept -> const FieldDescriptor& {
    return descriptor_;
  }

  auto data(this auto&& self) noexcept -> decltype(auto) {
    return std::forward_like<decltype(self)>(self.data_);
  }

private:

  FieldDescriptor descriptor_;
  std::vector<std::byte> data_;

}; // class FieldData

namespace impl {

class RunPublisherState;
class ParticleFileWriterState;
class RunReaderState;

template<class Val, bool = std::is_enum_v<Val>>
struct storage_value : std::type_identity<Val> {};

template<class Val>
struct storage_value<Val, true> : std::underlying_type<Val> {};

template<class Val>
using storage_value_t = storage_value<Val>::type;

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Builder for one unpublished HDF5 frame.
class FrameWriter final {
public:

  FrameWriter(const FrameWriter&) = delete;
  auto operator=(const FrameWriter&) -> FrameWriter& = delete;
  FrameWriter(FrameWriter&&) noexcept = default;
  auto operator=(FrameWriter&&) noexcept -> FrameWriter& = default;

  /// Abandon an uncommitted partial frame.
  ~FrameWriter();

  /// Write a contiguous typed field directly to the HDF5 dataset.
  template<std::ranges::contiguous_range Range>
    requires std::ranges::sized_range<Range> &&
             (known_type_of<std::ranges::range_value_t<Range>> ||
              std::is_enum_v<std::ranges::range_value_t<Range>>)
  void write(std::string_view name, const Range& values) {
    using Value = std::ranges::range_value_t<Range>;
    using StorageValue = impl::storage_value_t<Value>;
    static_assert(known_type_of<StorageValue>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(sizeof(Value) == type_of<StorageValue>.width());

    const std::span<const Value> span{std::ranges::data(values),
                                      std::ranges::size(values)};
    write_(name, type_of<StorageValue>, std::as_bytes(span), span.size());
  }

  /// Atomically publish the completed frame and update the run index.
  void commit();

private:

  friend class RunWriter;

  explicit FrameWriter(
      std::shared_ptr<impl::ParticleFileWriterState> state) noexcept;
  void write_(std::string_view name,
              Type type,
              std::span<const std::byte> data,
              std::size_t size);

  std::shared_ptr<impl::ParticleFileWriterState> state_;

}; // class FrameWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Builder for one unpublished lossless restart checkpoint.
class CheckpointWriter final {
public:

  CheckpointWriter(const CheckpointWriter&) = delete;
  auto operator=(const CheckpointWriter&) -> CheckpointWriter& = delete;
  CheckpointWriter(CheckpointWriter&&) noexcept = default;
  auto operator=(CheckpointWriter&&) noexcept -> CheckpointWriter& = default;
  ~CheckpointWriter();

  /// Write a contiguous typed persistent field to the checkpoint.
  template<std::ranges::contiguous_range Range>
    requires std::ranges::sized_range<Range> &&
             (known_type_of<std::ranges::range_value_t<Range>> ||
              std::is_enum_v<std::ranges::range_value_t<Range>>)
  void write(std::string_view name, const Range& values) {
    using Value = std::ranges::range_value_t<Range>;
    using StorageValue = impl::storage_value_t<Value>;
    static_assert(known_type_of<StorageValue>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(sizeof(Value) == type_of<StorageValue>.width());

    const std::span<const Value> span{std::ranges::data(values),
                                      std::ranges::size(values)};
    write_(name, type_of<StorageValue>, std::as_bytes(span), span.size());
  }

  /// Atomically publish the completed checkpoint.
  void commit();

private:

  friend class RunWriter;

  explicit CheckpointWriter(
      std::shared_ptr<impl::ParticleFileWriterState> state) noexcept;
  void write_(std::string_view name,
              Type type,
              std::span<const std::byte> data,
              std::size_t size);

  std::shared_ptr<impl::ParticleFileWriterState> state_;

}; // class CheckpointWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Writer for a new, versioned `.tit-run` directory.
class RunWriter final {
public:

  /// Create a new run directory. Existing non-empty paths are rejected.
  explicit RunWriter(std::filesystem::path path, RunMetadata metadata);

  /// Begin an unpublished frame. Only one frame may be active at a time.
  auto begin_frame(std::uint64_t step, double time) -> FrameWriter;

  /// Begin an unpublished lossless restart checkpoint.
  auto begin_checkpoint(std::uint64_t step, double time) -> CheckpointWriter;

  /// Run directory path.
  auto path() const -> const std::filesystem::path&;

private:

  std::shared_ptr<impl::RunPublisherState> state_;

}; // class RunWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reader for one immutable, committed HDF5 frame.
class FrameReader final {
public:

  /// Frame metadata from the committed index.
  auto descriptor() const noexcept -> const FrameDescriptor&;

  /// Enumerate the frame field schema.
  auto fields() const -> std::vector<FieldDescriptor>;

  /// Read a field as a descriptor and contiguous bytes.
  auto read_field(std::string_view name) const -> FieldData;

  /// Read and validate a field as a concrete C++ value type.
  template<known_type_of Val>
  auto read(std::string_view name) const -> std::vector<Val> {
    static_assert(std::is_trivially_copyable_v<Val>);
    static_assert(sizeof(Val) == type_of<Val>.width());
    auto field = read_field(name);
    TIT_ENSURE(field.descriptor().type() == type_of<Val>,
               "Field '{}' has type '{}', not '{}'.",
               name,
               field.descriptor().type().name(),
               type_of<Val>.name());
    std::vector<Val> values(field.descriptor().size());
    if (!field.data().empty()) {
      std::memcpy(values.data(), field.data().data(), field.data().size());
    }
    return values;
  }

private:

  friend class RunReader;

  FrameReader(std::filesystem::path path, FrameDescriptor descriptor);

  std::filesystem::path path_;
  FrameDescriptor descriptor_;

}; // class FrameReader

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reader for one immutable committed restart checkpoint.
class CheckpointReader final {
public:

  /// Checkpoint metadata from the committed index.
  auto descriptor() const noexcept -> const FrameDescriptor&;

  /// Enumerate persistent checkpoint fields.
  auto fields() const -> std::vector<FieldDescriptor>;

  /// Read a field as a descriptor and contiguous bytes.
  auto read_field(std::string_view name) const -> FieldData;

  /// Read and validate a persistent field as a concrete C++ value type.
  template<known_type_of Val>
  auto read(std::string_view name) const -> std::vector<Val> {
    static_assert(std::is_trivially_copyable_v<Val>);
    static_assert(sizeof(Val) == type_of<Val>.width());
    auto field = read_field(name);
    TIT_ENSURE(field.descriptor().type() == type_of<Val>,
               "Field '{}' has type '{}', not '{}'.",
               name,
               field.descriptor().type().name(),
               type_of<Val>.name());
    std::vector<Val> values(field.descriptor().size());
    if (!field.data().empty()) {
      std::memcpy(values.data(), field.data().data(), field.data().size());
    }
    return values;
  }

private:

  friend class RunReader;

  CheckpointReader(std::filesystem::path path, FrameDescriptor descriptor);

  std::filesystem::path path_;
  FrameDescriptor descriptor_;

}; // class CheckpointReader

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reader for committed frames in a `.tit-run` directory.
class RunReader final {
public:

  /// Open and validate an existing run directory.
  explicit RunReader(std::filesystem::path path);

  /// Refresh the immutable-frame index while a simulation is running.
  void refresh();

  /// Run metadata.
  auto metadata() const -> const RunMetadata&;

  /// Number of committed frames currently visible.
  auto num_frames() const -> std::size_t;

  /// Open a committed frame by logical index.
  auto frame(std::size_t index) const -> FrameReader;

  /// Number of committed checkpoints currently visible.
  auto num_checkpoints() const -> std::size_t;

  /// Open a committed checkpoint by logical index.
  auto checkpoint(std::size_t index) const -> CheckpointReader;

  /// Atomically copy the currently visible committed run to a new path.
  void copy_to(const std::filesystem::path& destination) const;

  /// Run directory path.
  auto path() const -> const std::filesystem::path&;

private:

  std::shared_ptr<impl::RunReaderState> state_;

}; // class RunReader

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io
