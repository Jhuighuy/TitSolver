/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/meta.hpp"
#pragma once

#include <concepts>
#include <functional>

#include "tit/core/basic_types.hpp"
#include "tit/core/meta/type.hpp"
#include "tit/core/type_traits.hpp"

namespace tit::meta {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class for meta type collections.
template<type... Ts>
class Range {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Call a function with all elements.
  template<std::invocable<Ts...> Func>
  constexpr auto apply(Func func) const -> decltype(auto) {
    return std::invoke(func, Ts{}...);
  }

  /// Call a function for each element.
  template<class Func>
    requires (std::invocable<Func, Ts> && ...)
  constexpr void for_each(Func func) const {
    (std::invoke(func, Ts{}), ...);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if the range contains a type `U`.
  template<type U>
  constexpr auto contains(U /*elem*/) const noexcept -> bool {
    return contains_v<U, Ts...>;
  }

  /// Check if the range contains all types in the range `Us...`.
  template<type... Us>
  constexpr auto includes(const Range<Us...>& /*rng*/) const noexcept -> bool {
    return (contains_v<Us, Ts...> && ...);
  }

  /// Index of the type in the set.
  template<type U>
    requires contains_v<U, Ts...>
  constexpr auto find(U /*elem*/) const noexcept -> size_t {
    return index_of_v<U, Ts...>;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

protected:

  /// Construct a range.
  consteval Range() = default;

}; // class Range

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::meta
