/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <tuple>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/serialization.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Array serializer.
template<serializable Val, size_t Size>
struct Serializer<std::array<Val, Size>> final {
  template<serialization_iterator OutIter>
  constexpr auto operator()(const std::array<Val, Size>& array,
                            OutIter out) const -> OutIter {
    for (const auto& elem : array) out = serialize(elem, out);
    return out;
  }
  template<deserialization_iterator InIter>
  constexpr auto operator()(std::array<Val, Size>& array,
                            InIter& iter,
                            InIter last) const -> bool {
    for (auto& elem : array) {
      if (!deserialize(elem, iter, last)) return false;
    }
    return true;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Pair serializer.
template<serializable First, serializable Second>
struct Serializer<std::pair<First, Second>> final {
  template<serialization_iterator OutIter>
  constexpr auto operator()(const std::pair<First, Second>& pair,
                            OutIter out) const -> OutIter {
    out = serialize(pair.first, out);
    return serialize(pair.second, out);
  }
  template<deserialization_iterator InIter>
  constexpr auto operator()(std::pair<First, Second>& pair,
                            InIter& iter,
                            InIter last) const -> bool {
    if (!deserialize(pair.first, iter, last)) return false;
    return deserialize(pair.second, iter, last);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Tuple serializer.
template<serializable... Vals>
struct Serializer<std::tuple<Vals...>> final {
  template<serialization_iterator OutIter>
  constexpr auto operator()(const std::tuple<Vals...>& tuple,
                            OutIter out) const -> OutIter {
    return std::apply(
        [&out](const auto&... elems) {
          return ((out = serialize(elems, out)), ...);
        },
        tuple);
  }
  template<deserialization_iterator InIter>
  constexpr auto operator()(std::tuple<Vals...>& tuple,
                            InIter& iter,
                            InIter last) const -> bool {
    return std::apply(
        [&iter, last](auto&... elems) {
          return ((deserialize(elems, iter, last) && ...), true);
        },
        tuple);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
