/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract source of the serialized bytes.
class DataSource : public VirtualBase {
public:

  /// Pull the at most `count` bytes from the data source.
  ///
  /// @returns A view of the pulled bytes. The data is owned by the data
  /// source,
  ///          and the view is valid until the next call to `pull` or
  ///          destruction of the data source.
  virtual auto pull(size_t count) -> std::span<const byte_t> = 0;

}; // class DataSource

/// Source of the serialized bytes that serializes the values of a range.
template<std::ranges::view Data>
  requires serializable_range<Data>
class RangeDataSource final : public DataSource {
public:

  /// Construct a data source.
  constexpr explicit RangeDataSource(Data data) noexcept
      : data_{std::move(data)}, iter_{std::begin(data_)} {}

  /// Pull the at most `count` bytes from the data source.
  constexpr auto pull(size_t count) -> std::span<const byte_t> override {
    // Remove the data that have been pulled before.
    const auto last_count = std::exchange(last_count_, count);
    buffer_.erase(buffer_.begin(), buffer_.begin() + last_count);

    // Pull the new data from the range into the buffer.
    for (; buffer_.size() < count && iter_ != std::end(data_); ++iter_) {
      serialize(*iter_, std::back_inserter(buffer_));
    }
    return {std::begin(buffer_), std::min(count, buffer_.size())};
  }

private:

  Data data_;
  std::ranges::iterator_t<Data> iter_;
  std::vector<byte_t> buffer_;
  size_t last_count_ = 0;

}; // class RangeDataSource

template<std::ranges::viewable_range Data>
RangeDataSource(Data) -> RangeDataSource<std::views::all_t<Data>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract sink to push the serialized bytes.
class DataSink : public VirtualBase {
public:

  /// Push the bytes to the data sink.
  virtual void push(std::span<const byte_t> bytes) = 0;

}; // class DataSink

/// Sink for the serialized bytes that deserializes the values and writes them
/// into the iterator.
template<serializable Val, std::output_iterator<Val> Iter>
class IterDataSink final : public DataSink {
public:

  /// Construct a data sink.
  constexpr explicit IterDataSink(Iter iter) noexcept : iter_{iter} {}

  /// Push the bytes to the data sink.
  constexpr void push(std::span<const byte_t> bytes) override {
    for (Val val{}; const auto& byte : bytes) {
      buffer_.push_back(byte);
      auto iter = buffer_.begin();
      if (deserialize(val, iter, buffer_.end())) {
        *iter_ = std::move(val);
        ++iter_;
        buffer_.clear();
      }
    }
  }

private:

  Iter iter_;
  std::vector<byte_t> buffer_;

}; // class TypedDataSink

/// Make an iterator data sink.
template<serializable Val = byte_t, std::output_iterator<Val> Iter>
constexpr auto make_iter_data_sink(Iter iter) -> IterDataSink<Val, Iter> {
  return IterDataSink<Val, Iter>{iter};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
