/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Helper object to store a string literal for non-type template parameters.
template<size_t Size>
class StringLiteral final {
public:

  /// String data.
  // NOLINTNEXTLINE(*-non-private-member-variables-in-classes)
  std::array<char, Size> data;

  /// Construct the string literal from a character array.
  consteval explicit(false) StringLiteral(carr_ref_t<const char, Size> str) {
    std::copy_n(str, Size, data.begin());
  }

}; // class StringLiteral

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Transparent string hash function.
struct StringHash {
  using is_transparent = void; // enables heterogeneous operations.
  auto operator()(std::string_view string) const noexcept -> size_t {
    constexpr std::hash<std::string_view> hasher{};
    return hasher(string);
  }
}; // struct StringHash

/// String hash set.
using StringHashSet =
    std::unordered_set<std::string, StringHash, std::equal_to<>>;

/// String hash map.
template<class Val>
using StringHashMap =
    std::unordered_map<std::string, Val, StringHash, std::equal_to<>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
