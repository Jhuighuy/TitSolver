/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits> // IWYU pragma: keep -- IWYU does not recognize concepts.

#include "tit/core/types.hpp"

namespace tit::meta {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** An empty object. */
template<class T>
concept meta_type = true;
template<class T>
concept type = std::is_object_v<T> && std::is_empty_v<T>;

/** Check that T is in Us. */
template<meta_type T, meta_type... Us>
inline constexpr bool contains_v = (... || std::same_as<T, Us>);

/** Index of T in list U, Us. */
template<meta_type T, meta_type U, meta_type... Us>
  requires contains_v<T, U, Us...>
inline constexpr size_t index_of_v = [] {
  if constexpr (std::same_as<T, U>) return 0;
  else return (index_of_v<T, Us...> + 1);
}();

/** Check that all Ts are unique. */
template<class... Ts>
inline constexpr bool all_unique_v = true;
// clang-format off
template<class T, class... Ts>
inline constexpr bool all_unique_v<T, Ts...> =
    (!contains_v<T, Ts...>) && all_unique_v<Ts...>;
// clang-format on

// Disable named parameters. This check does not make any sense here.
// NOLINTBEGIN(*-named-parameter)

template<meta_type... Ts>
  requires all_unique_v<Ts...>
class Set {
public:

  /** Construct a meta-set. */
  consteval Set() = default;
  consteval explicit Set(Ts...)
#ifndef __INTELLISENSE__
    requires (sizeof...(Ts) != 0)
#endif
  {
  }

  static consteval auto size() -> size_t {
    return sizeof...(Ts);
  }

  /** Set union operation. */
  consteval auto operator|(Set<>) const {
    return Set<Ts...>{};
  }
  template<meta_type U, meta_type... Us>
  consteval auto operator|(Set<U, Us...>) const {
    if constexpr (contains_v<U, Ts...>) return Set<Ts...>{} | Set<Us...>{};
    else return Set<Ts..., U>{} | Set<Us...>{};
  }

  /** Set minus operation. */
  consteval auto operator-(Set<>) const {
    return Set<Ts...>{};
  }
  template<meta_type U, meta_type... Us>
  consteval auto operator-(Set<U, Us...>) const {
    if constexpr (contains_v<U, Ts...>) {
      return []<class X, class... Xs>(Set<X, Xs...>) {
        return Set<Xs...>{} - Set<Us...>{};
      }(Set<U>{} | Set<Ts...>{});
    } else return Set<Ts...>{} - Set<Us...>{};
  }

  template<meta_type U>
  static consteval auto contains(U) -> bool {
    return contains_v<U, Ts...>;
  }

  template<meta_type... Us>
  static consteval auto includes(Set<Us...>) -> bool {
    return std::is_same_v<Set<Ts...>, decltype(Set<Ts...>{} | Set<Us...>{})>;
  }
};

// NOLINTEND(*-named-parameter)

template<class T>
inline constexpr bool is_set_v = false;
template<class... Ts>
inline constexpr bool is_set_v<Set<Ts...>> = true;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class T>
static consteval auto _type_name_impl() {
  return __PRETTY_FUNCTION__;
}

template<class T>
inline constexpr auto type_name = _type_name_impl<T>();

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::meta
