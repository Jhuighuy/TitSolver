/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

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

} // namespace impl

/// Tuple-like type.
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
