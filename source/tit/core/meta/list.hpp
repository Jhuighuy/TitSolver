/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/meta/range.hpp"
#include "tit/core/meta/type.hpp"

namespace tit::meta {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// List of meta types.
template<type... Ts>
class List final : public Range<Ts...> {
public:

  /// Construct a list.
  /// @{
  consteval List() noexcept = default;
  consteval explicit List(Ts... /*elems*/)
    requires (sizeof...(Ts) != 0)
  {}
  /// @}

  /// Concatenate the lists.
  ///
  /// @returns A list that contains all the elements of @p lhs followed by the
  ///          elements of @p rhs that are not already present in @p lhs.
  template<type... Us>
  friend constexpr auto operator+(List /*lhs*/, List<Us...> /*rhs*/) noexcept {
    return List<Ts..., Us...>{};
  }

  /// Check that @p lhs and @p rhs contain same elements.
  template<type... Us>
  friend constexpr auto operator==(List /*lhs*/,
                                   List<Us...> /*rhs*/) noexcept -> bool {
    return std::is_same_v<List<Ts...>, List<Us...>>;
  }

}; // class List

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<type... Ts>
constexpr auto cartesian_product(List<Ts...> lhs) noexcept {
  return lhs;
}
template<type... Ts, type... Us>
constexpr auto cartesian_product(List<Ts...> /*lhs*/,
                                 List<Us...> /*rhs*/) noexcept {
  constexpr auto zip_with_rhs = []<type T>(T elem) {
    return List{(List{elem} + List{Us{}})...};
  };
  return (zip_with_rhs(Ts{}) + ...);
}
template<type... Ts, class... RestLists>
constexpr auto cartesian_product(List<Ts...> list,
                                 RestLists... rest_lists) noexcept {
  return cartesian_product(list, cartesian_product(rest_lists...));
}

} // namespace impl

/// Compute the cartesian product of the lists.
///
/// @returns A list of lists, where each list is a list of elements from the
///          input lists.
template<class... Lists>
constexpr auto cartesian_product(Lists... lists) noexcept {
  return impl::cartesian_product(lists...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::meta
