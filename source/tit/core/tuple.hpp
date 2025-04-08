/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <algorithm>
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

/// Array type that is deduced from the given argument.
/// If the argument is not an array, it is wrapped into an array of size 1.
template<class Arg>
using array_from_arg_t = decltype(std::array{std::declval<Arg>()});

/// Size of the array that is constructed by packing the given values
/// or arrays of values into a single array.
template<class... Args>
inline constexpr auto packed_array_size_v =
    std::tuple_size_v<decltype(std::tuple_cat(array_from_arg_t<Args>{}...))>;

/// Check if the given values or arrays of values can be packed into a single
/// array of the given size and type.
template<size_t Size, class R, class... Args>
concept can_pack_array =
    (... &&
     std::constructible_from<R, typename array_from_arg_t<Args>::value_type>) &&
    (((... + std::tuple_size_v<array_from_arg_t<Args>>) == Size) ||
     (((... + std::tuple_size_v<array_from_arg_t<Args>>) < Size) &&
      std::default_initializable<R>) );

/// Pack values and arrays of values into an array of the given size and type.
template<size_t Size, class R, class... Args>
  requires can_pack_array<Size, R, Args...>
constexpr auto make_array(Args&&... args) -> std::array<R, Size> {
  return std::apply(
      []<class... Elems>(Elems&&... elems) {
        return std::array<R, Size>{R(std::forward<Elems>(elems))...};
      },
      std::tuple_cat(std::array{std::forward<Args>(args)}...));
}

/// Concatenate arrays.
template<size_t... Sizes, class T>
  requires std::copy_constructible<T>
constexpr auto array_cat(const std::array<T, Sizes>&... arrays)
    -> std::array<T, (... + Sizes)> {
  std::array<T, (... + Sizes)> result{};
  auto out_iter = result.begin();
  (..., [&out_iter](const auto& array) {
    out_iter = std::ranges::copy(array, out_iter).out;
  }(arrays));
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A pair of values of the same type.
template<class T>
using pair_of_t = std::pair<T, T>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// C-style array reference.
template<class T, size_t Size>
using carr_ref_t = T (&)[Size]; // NOLINT(*-c-arrays)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
