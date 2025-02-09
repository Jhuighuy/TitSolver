/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"
#include "tit/core/type_utils.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Throw an exception indicating that the deserialization failed due to
/// truncated stream.
[[noreturn]] constexpr auto deserialization_failed() -> bool {
  TIT_THROW("Serialization failed: truncated stream!");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a trivially-copyable value into the output stream.
template<class Stream, class Item>
  requires (std::is_trivially_copyable_v<Item> &&
            (std::integral<Item> || std::floating_point<Item>) )
constexpr void serialize(Stream& out, const Item& item) {
  out.write(to_byte_array(item));
}

/// Deserialize a trivially-copyable value from the input stream.
template<class Stream, class Item>
  requires (std::is_trivially_copyable_v<Item> &&
            (std::integral<Item> || std::floating_point<Item>) )
constexpr auto deserialize(Stream& in, Item& item) -> bool {
  std::array<byte_t, sizeof(Item)> bytes{};
  const auto copied = in.read(bytes);
  if (copied == 0) return false;
  if (copied != bytes.size()) deserialization_failed();
  item = from_bytes<Item>(bytes);
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize multiple values into the output stream.
template<class Stream, class... Items>
  requires (sizeof...(Items) > 1)
constexpr void serialize(Stream& out, const Items&... items) {
  (serialize(out, items), ...);
}

/// Deserialize multiple values from the input stream.
template<class Stream, class FirstItem, class... RestItems>
  requires (sizeof...(RestItems) > 0)
constexpr auto deserialize(Stream& in,
                           FirstItem& first_item,
                           RestItems&... rest_items) -> bool {
  if (!deserialize(in, first_item)) return false;
  if ((deserialize(in, rest_items) && ...)) return true;
  deserialization_failed();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a tuple of values into the output stream.
template<tuple_like Tuple, class Stream>
constexpr void serialize(Stream& out, const Tuple& tuple) {
  std::apply([&out](const auto&... items) { serialize(out, items...); }, tuple);
}

/// Deserialize a tuple of values from the input stream.
template<tuple_like Tuple, class Stream>
constexpr auto deserialize(Stream& in, Tuple& tuple) -> bool {
  return std::apply(
      [&in](auto&... items) { return (deserialize(in, items...)); },
      tuple);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that serializes the values and writes them to the underlying stream.
template<class Item>
class StreamSerializer final : public OutputStream<Item> {
public:

  /// Construct a stream serializer.
  constexpr explicit StreamSerializer(OutputStreamPtr<byte_t> stream) noexcept
      : stream_{std::move(stream)} {}

  /// Serialize the items and write them to the stream.
  constexpr void write(std::span<const Item> items) override {
    TIT_ASSERT(stream_ != nullptr, "Stream is null!");
    for (const auto& item : items) serialize(*stream_, item);
  }

  /// Flush the stream.
  constexpr auto flush() -> void override {
    stream_->flush();
  }

private:

  OutputStreamPtr<byte_t> stream_;

}; // class StreamSerializer

/// Make a stream serializer.
template<class Item>
constexpr auto make_stream_serializer(OutputStreamPtr<byte_t> stream)
    -> OutputStreamPtr<Item> {
  return make_flushable<StreamSerializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that deserializes the values and reads them from the underlying
/// stream.
template<class Item>
class StreamDeserializer final : public InputStream<Item> {
public:

  /// Construct a stream deserializer.
  constexpr explicit StreamDeserializer(InputStreamPtr<byte_t> stream) noexcept
      : stream_{std::move(stream)} {}

  /// Read the bytes from the stream and deserialize thems.
  constexpr auto read(std::span<Item> items) -> size_t override {
    TIT_ASSERT(stream_ != nullptr, "Stream is null!");
    for (size_t i = 0; i < items.size(); ++i) {
      if (!deserialize(*stream_, items[i])) {
        if (byte_t probe{}; stream_->read({&probe, 1}) != 1) return i;
        deserialization_failed();
      }
    }
    return items.size();
  }

private:

  InputStreamPtr<byte_t> stream_;

}; // class StreamDeserializer

/// Make a stream deserializer.
template<class Item>
constexpr auto make_stream_deserializer(InputStreamPtr<byte_t> stream)
    -> InputStreamPtr<Item> {
  return std::make_unique<StreamDeserializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
