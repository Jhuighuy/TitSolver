/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>

#include "tit/dist/communicator.hpp"
#include "tit/io/run.hpp"
#include "tit/io/type.hpp"

namespace tit::io {

namespace impl {

class ParallelRunWriterState;
class ParallelParticleFileWriterState;

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Collective builder for one unpublished parallel HDF5 frame.
class ParallelFrameWriter final {
public:

  ParallelFrameWriter(const ParallelFrameWriter&) = delete;
  auto operator=(const ParallelFrameWriter&) -> ParallelFrameWriter& = delete;
  ParallelFrameWriter(ParallelFrameWriter&&) noexcept = default;
  auto operator=(ParallelFrameWriter&&) noexcept
      -> ParallelFrameWriter& = default;
  ~ParallelFrameWriter();

  /// Collectively write one rank-local contiguous field hyperslab.
  template<std::ranges::contiguous_range Range>
    requires std::ranges::sized_range<Range> &&
             (known_type_of<std::ranges::range_value_t<Range>> ||
              std::is_enum_v<std::ranges::range_value_t<Range>>)
  void write(std::string_view name, const Range& local_values) {
    using Value = std::ranges::range_value_t<Range>;
    using StorageValue = impl::storage_value_t<Value>;
    static_assert(known_type_of<StorageValue>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(sizeof(Value) == type_of<StorageValue>.width());

    const std::span<const Value> values{std::ranges::data(local_values),
                                        std::ranges::size(local_values)};
    write_(name, type_of<StorageValue>, std::as_bytes(values), values.size());
  }

  /// Collectively close and atomically publish the completed frame.
  void commit();

private:

  friend class ParallelRunWriter;

  explicit ParallelFrameWriter(
      std::shared_ptr<impl::ParallelParticleFileWriterState> state) noexcept;

  void write_(std::string_view name,
              Type type,
              std::span<const std::byte> local_data,
              std::size_t local_size);

  std::shared_ptr<impl::ParallelParticleFileWriterState> state_;

}; // class ParallelFrameWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Collective builder for one unpublished restart checkpoint.
class ParallelCheckpointWriter final {
public:

  ParallelCheckpointWriter(const ParallelCheckpointWriter&) = delete;
  auto operator=(const ParallelCheckpointWriter&)
      -> ParallelCheckpointWriter& = delete;
  ParallelCheckpointWriter(ParallelCheckpointWriter&&) noexcept = default;
  auto operator=(ParallelCheckpointWriter&&) noexcept
      -> ParallelCheckpointWriter& = default;
  ~ParallelCheckpointWriter();

  /// Collectively write one rank-local persistent field hyperslab.
  template<std::ranges::contiguous_range Range>
    requires std::ranges::sized_range<Range> &&
             (known_type_of<std::ranges::range_value_t<Range>> ||
              std::is_enum_v<std::ranges::range_value_t<Range>>)
  void write(std::string_view name, const Range& local_values) {
    using Value = std::ranges::range_value_t<Range>;
    using StorageValue = impl::storage_value_t<Value>;
    static_assert(known_type_of<StorageValue>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(sizeof(Value) == type_of<StorageValue>.width());

    const std::span<const Value> values{std::ranges::data(local_values),
                                        std::ranges::size(local_values)};
    write_(name, type_of<StorageValue>, std::as_bytes(values), values.size());
  }

  /// Collectively close and atomically publish the completed checkpoint.
  void commit();

private:

  friend class ParallelRunWriter;

  explicit ParallelCheckpointWriter(
      std::shared_ptr<impl::ParallelParticleFileWriterState> state) noexcept;

  void write_(std::string_view name,
              Type type,
              std::span<const std::byte> local_data,
              std::size_t local_size);

  std::shared_ptr<impl::ParallelParticleFileWriterState> state_;

}; // class ParallelCheckpointWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// MPI-collective writer for a versioned `.tit-run` directory.
class ParallelRunWriter final {
public:

  explicit ParallelRunWriter(std::filesystem::path path,
                             RunMetadata metadata,
                             dist::Communicator communicator);

  /// Collectively begin one frame. Every rank must call fields in the same
  /// order.
  auto begin_frame(std::uint64_t step, double time) -> ParallelFrameWriter;

  /// Collectively begin one lossless, rank-count-independent checkpoint.
  auto begin_checkpoint(std::uint64_t step, double time)
      -> ParallelCheckpointWriter;

  auto path() const -> const std::filesystem::path&;

private:

  std::shared_ptr<impl::ParallelRunWriterState> state_;

}; // class ParallelRunWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io
