/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/stream.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Encode a byte array into a Base64 string.
auto encode_base64(std::span<const std::byte> data) -> std::string;

/// Decode a Base64 string into a byte array.
auto decode_base64(std::string_view data) -> std::vector<std::byte>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a value to a byte array.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_byte_array(const T& value)
    -> std::array<std::byte, sizeof(T)> {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
}

/// Convert a value to a byte vector.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_bytes(const T& value) -> std::vector<std::byte> {
  const auto bytes_array = to_byte_array(value);
  return {bytes_array.begin(), bytes_array.end()};
}

/// Convert a byte array to a value.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto from_bytes(std::span<const std::byte> bytes) -> T {
  TIT_ASSERT(bytes.size() >= sizeof(T), "Invalid byte array size!");
  std::array<std::byte, sizeof(T)> bytes_array{};
  std::ranges::copy(bytes, bytes_array.begin());
  return std::bit_cast<T>(bytes_array);
}

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
  std::array<std::byte, sizeof(Item)> bytes{};
  const auto copied = in.read(bytes);
  if (copied == 0) return false;
  if (copied != bytes.size()) deserialization_failed();
  item = from_bytes<Item>(bytes);
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Tuple>
concept has_tuple_size =
    requires { typename std::tuple_size<Tuple>::type; } &&
    std::derived_from<std::tuple_size<Tuple>,
                      std::integral_constant<size_t, std::tuple_size_v<Tuple>>>;

template<class Tuple, size_t Index>
concept has_tuple_element = //
    requires {
      typename std::tuple_element_t<Index, std::remove_const_t<Tuple>>;
    } && requires (Tuple& tuple) {
      {
        std::get<Index>(tuple)
      } -> std::convertible_to<const std::tuple_element_t<Index, Tuple>&>;
    };

template<class Tuple, size_t Size>
concept tuple_size_is =
    has_tuple_size<Tuple> && (std::tuple_size_v<Tuple> == Size);

template<class Tuple, size_t Index, class Elem>
concept tuple_element_is =
    has_tuple_element<Tuple, Index> &&
    std::constructible_from<Elem, std::tuple_element_t<Index, Tuple>>;

template<class Tuple, class... Items>
concept tuple_like =
    std::is_object_v<Tuple> && impl::has_tuple_size<Tuple> &&
    []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
      return (impl::has_tuple_element<Tuple, Indices> && ...);
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>()) &&
    ((sizeof...(Items) == 0) ||
     (impl::tuple_size_is<Tuple, sizeof...(Items)> &&
      []<size_t... Indices>(std::index_sequence<Indices...> /*indices*/) {
        return (impl::tuple_element_is<Tuple, Indices, Items> && ...);
      }(std::make_index_sequence<sizeof...(Items)>())));

} // namespace impl

/// Serialize a tuple of values into the output stream.
template<impl::tuple_like Tuple, class Stream>
constexpr void serialize(Stream& out, const Tuple& tuple) {
  std::apply([&out](const auto&... items) { serialize(out, items...); }, tuple);
}

/// Deserialize a tuple of values from the input stream.
template<impl::tuple_like Tuple, class Stream>
constexpr auto deserialize(Stream& in, Tuple& tuple) -> bool {
  return std::apply(
      [&in](auto&... items) { return (deserialize(in, items...)); },
      tuple);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a vector into the output stream.
template<class Stream, class Num, size_t Dim>
constexpr void serialize(Stream& out, const Vec<Num, Dim>& vec) {
  serialize(out, vec.elems());
}

/// Deserialize a vector from the input stream.
template<class Stream, class Num, size_t Dim>
constexpr auto deserialize(Stream& in, Vec<Num, Dim>& vec) -> bool {
  return deserialize(in, vec.elems());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a matrix into the output stream.
template<class Stream, class Num, size_t Dim>
constexpr void serialize(Stream& out, const Mat<Num, Dim>& mat) {
  serialize(out, mat.rows());
}

/// Deserialize a matrix from the input stream.
template<class Stream, class Num, size_t Dim>
constexpr auto deserialize(Stream& in, Mat<Num, Dim>& mat) -> bool {
  return deserialize(in, mat.rows());
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

/// Stream that serializes the values and writes them to the underlying stream.
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
  return make_flushable<StreamSerializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that deserializes the values and reads them from the underlying
/// stream.
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
      if (!deserialize(*stream_, items[i])) {
        if (std::byte probe{}; stream_->read({&probe, 1}) != 1) return i;
        deserialization_failed();
      }
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
  return std::make_unique<StreamDeserializer<Item>>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
