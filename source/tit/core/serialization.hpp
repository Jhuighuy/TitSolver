/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/stream.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a value to a byte array.
template<class T>
  requires std::is_trivially_destructible_v<T>
constexpr auto to_byte_array(const T& value)
    -> std::array<std::byte, sizeof(T)> {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
}

/// Convert a value to a byte vector.
template<class T>
  requires std::is_trivially_destructible_v<T>
constexpr auto to_bytes(const T& value) -> std::vector<std::byte> {
  const auto bytes_array = to_byte_array(value);
  return {bytes_array.begin(), bytes_array.end()};
}

/// Convert a byte array to a value.
template<class T>
  requires std::is_trivially_destructible_v<T>
constexpr auto from_bytes(std::span<const std::byte> bytes) -> T {
  TIT_ASSERT(bytes.size() >= sizeof(T), "Invalid byte array size!");
  std::array<std::byte, sizeof(T)> bytes_array{};
  std::ranges::copy(bytes, bytes_array.begin());
  return std::bit_cast<T>(bytes_array);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a trivially-destructible value into the output stream.
template<class Val>
  requires (std::is_trivially_destructible_v<Val> &&
            (std::integral<Val> || std::floating_point<Val>) )
constexpr void serialize(OutputStream<std::byte>& out, const Val& val) {
  out.write(to_byte_array(val));
}

/// Deserialize a trivially-destructible value from the input stream.
template<class Val>
  requires (std::is_trivially_destructible_v<Val> &&
            (std::integral<Val> || std::floating_point<Val>) )
constexpr auto deserialize(InputStream<std::byte>& in, Val& val) -> bool {
  std::array<std::byte, sizeof(Val)> bytes{};
  const auto copied = in.read(bytes);
  if (copied == 0) return false;
  TIT_ENSURE(copied == bytes.size(),
             "Deserialization failed: expected {} bytes, got {}.",
             bytes.size(),
             copied);
  val = from_bytes<Val>(bytes);
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a tuple of values into the output stream.
template<tuple_like Tuple>
constexpr void serialize(OutputStream<std::byte>& out, const Tuple& tuple) {
  std::apply([&out](const auto&... items) { (serialize(out, items), ...); },
             tuple);
}

/// Deserialize a tuple of values from the input stream.
template<tuple_like Tuple>
constexpr auto deserialize(InputStream<std::byte>& in, Tuple& tuple) -> bool {
  return std::apply(
      [&in](auto& first_item, auto&... rest_items) {
        if (!deserialize(in, first_item)) return false;
        if ((deserialize(in, rest_items) && ...)) return true;
        TIT_THROW("Deserialization failed: failed to deserialize {} items.",
                  sizeof...(rest_items) + 1);
      },
      tuple);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a vector into the output stream.
template<class Num, size_t Dim>
constexpr void serialize(OutputStream<std::byte>& out,
                         const Vec<Num, Dim>& vec) {
  serialize(out, vec.elems());
}

/// Deserialize a vector from the input stream.
template<class Num, size_t Dim>
constexpr auto deserialize(InputStream<std::byte>& in, Vec<Num, Dim>& vec)
    -> bool {
  return deserialize(in, vec.elems());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a matrix into the output stream.
template<class Num, size_t Dim>
constexpr void serialize(OutputStream<std::byte>& out,
                         const Mat<Num, Dim>& mat) {
  serialize(out, mat.rows());
}

/// Deserialize a matrix from the input stream.
template<class Num, size_t Dim>
constexpr auto deserialize(InputStream<std::byte>& in, Mat<Num, Dim>& mat)
    -> bool {
  return deserialize(in, mat.rows());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that serializes the values and writes them to the underlying byte
/// stream.
template<class Item>
class StreamSerializer final : public OutputStream<Item> {
public:

  /// Construct a stream serializer.
  constexpr explicit StreamSerializer(
      OutputStreamPtr<std::byte> stream) noexcept
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

  OutputStreamPtr<std::byte> stream_;

}; // class StreamSerializer

/// Make a stream serializer.
template<class Item>
constexpr auto make_stream_serializer(OutputStreamPtr<std::byte> stream)
    -> OutputStreamPtr<Item> {
  return make_output_stream<StreamSerializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that deserializes the values and reads them from the underlying
/// byte stream.
template<class Item>
class StreamDeserializer final : public InputStream<Item> {
public:

  /// Construct a stream deserializer.
  constexpr explicit StreamDeserializer(
      InputStreamPtr<std::byte> stream) noexcept
      : stream_{std::move(stream)} {}

  /// Read the bytes from the stream and deserialize thems.
  constexpr auto read(std::span<Item> items) -> size_t override {
    TIT_ASSERT(stream_ != nullptr, "Stream is null!");
    for (size_t i = 0; i < items.size(); ++i) {
      if (!deserialize(*stream_, items[i])) return i;
    }
    return items.size();
  }

private:

  InputStreamPtr<std::byte> stream_;

}; // class StreamDeserializer

/// Make a stream deserializer.
template<class Item>
constexpr auto make_stream_deserializer(InputStreamPtr<std::byte> stream)
    -> InputStreamPtr<Item> {
  return make_input_stream<StreamDeserializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
