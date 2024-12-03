/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Output byte iterator type to store the serialized bytes.
template<class Iter>
concept serialization_iterator = std::output_iterator<Iter, byte_t>;

/// Input byte iterator type to read the serialized bytes.
template<class Iter>
concept deserialization_iterator =
    std::input_iterator<Iter> && std::same_as<std::iter_value_t<Iter>, byte_t>;

/// Serialize or deserialize a value.
/// User must specialize this class to provide serialization for custom types.
template<class Val>
struct Serializer final {
  template<serialization_iterator OutIter>
  constexpr auto operator()(const Val& val,
                            OutIter out) const -> OutIter = delete;
  template<deserialization_iterator InIter>
  constexpr auto operator()(Val& val,
                            InIter& iter,
                            InIter last) const -> bool = delete;
  using not_specialized_ = void;
};

/// Serializable type.
template<class Val>
concept serializable =
    std::is_object_v<Val> && std::default_initializable<Val> &&
    (!requires { typename Serializer<Val>::not_specialized_; });

/// Range of serializable values.
template<class Range>
concept serializable_range = std::ranges::input_range<Range> &&
                             serializable<std::ranges::range_value_t<Range>>;

/// Serialize a value into the output iterator.
/// @{
template<serializable Val, serialization_iterator OutIter>
  requires std::invocable<Serializer<Val>, const Val&, OutIter>
constexpr auto serialize(Val val, OutIter out) -> OutIter {
  return Serializer<Val>{}(val, out);
}
template<serializable Val>
constexpr auto serialize(Val val) -> std::vector<byte_t> {
  std::vector<byte_t> result{};
  serialize(val, std::back_inserter(result));
  return result;
}
/// @}

/// Deserialize a value from the pair of iterators.
template<serializable Val, deserialization_iterator Iter>
  requires std::invocable<Serializer<Val>, Val&, Iter&, Iter>
[[nodiscard]]
constexpr auto deserialize(Val& val, Iter& iter, Iter last) -> bool {
  // It's easy to skip `&` when defining the deserializer, so we'll check
  // that the deserializer accepts both output value and iterator as
  // non-const references.
  static_assert(
      !std::invocable<Serializer<Val>, const Val&, Iter&, Iter>,
      "Deserializer must accept a non-const reference to the output value!");
  static_assert(
      !std::invocable<Serializer<Val>, Val&, const Iter&, Iter>,
      "Deserializer must accept a non-const reference to the iterator!");
  return Serializer<Val>{}(val, iter, last);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Trivially-serializable type. By trivial serialization we mean
/// directly copying the type's value bytes into the output buffer.
template<class T>
concept trivially_serializable =
    std::is_trivially_copyable_v<T> && // just in case.
    (std::integral<T> || std::floating_point<T> || std::is_enum_v<T>);

// Trivially-serializable type serializer.
template<trivially_serializable Val>
struct Serializer<Val> final {
  using ByteArray = std::array<byte_t, sizeof(Val)>;
  template<serialization_iterator OutIter>
  constexpr auto operator()(const Val& val, OutIter out) const -> OutIter {
    const auto result = std::ranges::copy(std::bit_cast<ByteArray>(val), out);
    return result.out;
  }
  template<deserialization_iterator InIter>
  constexpr auto operator()(Val& val, InIter& iter, InIter last) const -> bool {
    ByteArray bytes{};
    for (size_t b = 0; b < sizeof(Val); ++b, ++iter) {
      if (iter == last) return false; // Not enough bytes.
      bytes[b] = *iter;
    }
    val = std::bit_cast<Val>(bytes);
    return true;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
