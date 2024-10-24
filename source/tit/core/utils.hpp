/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wrap a macro argument with commas to pass it to another macro.
#define TIT_PASS(...) __VA_ARGS__

/// Concatenate macro arguments.
#define TIT_CAT(a, b) TIT_CAT_IMPL(a, b)
#define TIT_CAT_IMPL(a, b) a##b

/// Convert macro argument into a string literal.
#define TIT_STR(a) TIT_STR_IMPL(a)
#define TIT_STR_IMPL(a) #a

/// Generate a unique identifier
#define TIT_NAME(prefix) TIT_CAT(TIT_CAT(prefix, _), __LINE__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Use this function to assume forwarding references as universal references
/// to avoid false alarms from analysis tools.
/// @{
#define TIT_ASSUME_UNIVERSAL(T, ref) static_cast<void>(std::forward<T>(ref))
#define TIT_ASSUME_UNIVERSALS(Ts, refs) (TIT_ASSUME_UNIVERSAL(Ts, refs), ...)
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A pair of values of the same type.
template<class T>
using pair_of_t = std::pair<T, T>;

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

/// Deduce array type from the given arguments.
template<class... Ts>
using array_from_t = decltype(make_array(std::declval<Ts>()...));

/// Fill an array of the given size initialized with the given value.
template<size_t Size, class T>
  requires std::copy_constructible<T>
constexpr auto fill_array(const T& val) -> std::array<T, Size> {
  const auto get_val = [&val](auto /*arg*/) -> const T& { return val; };
  return [&get_val]<size_t... Indices>(std::index_sequence<Indices...>) {
    return std::array<T, Size>{get_val(Indices)...};
  }(std::make_index_sequence<Size>{});
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

/// Range of indices.
template<class Indices>
concept index_range = std::ranges::random_access_range<Indices> &&
                      std::integral<std::ranges::range_value_t<Indices>>;

/// Range of indices that can be used as output for various algorithms.
template<class Indices>
concept output_index_range = std::ranges::random_access_range<Indices> &&
                             std::ranges::output_range<Indices, size_t>;

/// Permuted range view.
template<std::ranges::random_access_range Range, index_range Perm>
  requires std::ranges::viewable_range<Range> &&
           std::ranges::viewable_range<Perm>
constexpr auto permuted_view(Range&& range, Perm&& perm) {
  return std::ranges::transform_view{
      std::forward<Perm>(perm),
      [range_view = std::views::all(std::forward<Range>(range))](
          size_t index) -> auto&& { return range_view[index]; }};
}

/// Initialize the permutation with the identity permutation.
/// @{
template<std::ranges::sized_range Range>
[[nodiscard]] constexpr auto iota_perm(Range&& range) noexcept {
  TIT_ASSUME_UNIVERSAL(Range, range);
  return std::views::iota(size_t{0}, std::size(range));
}
template<std::ranges::sized_range Range, output_index_range Perm>
constexpr void iota_perm(Range&& range, Perm&& perm) {
  TIT_ASSUME_UNIVERSAL(Range, range);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  std::ranges::copy(iota_perm(range), std::begin(perm));
}
/// @}

/// Copy the permutation into the output, filtering by the predicate that
/// is applied to the range items.
template<std::ranges::random_access_range Range,
         index_range Perm,
         std::output_iterator<size_t> OutIter,
         std::predicate<std::ranges::range_reference_t<Range&&>> Pred>
constexpr auto copy_perm_if(Range&& range, Perm&& perm, OutIter out, Pred pred)
    -> OutIter {
  TIT_ASSUME_UNIVERSAL(Range, range);
  TIT_ASSUME_UNIVERSAL(Perm, perm);
  const auto result = std::ranges::copy_if( //
      perm,
      out,
      pred,
      [&range](size_t i) -> auto&& { return range[i]; });
  return result.out;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Virtual base class.
class VirtualBase {
public:

  /// Construct the virtual class instance.
  VirtualBase() = default;

  /// Virtual class instance is not move-constructible.
  VirtualBase(VirtualBase&&) = delete;

  /// Virtual class instance is not copy-constructible.
  VirtualBase(const VirtualBase&) = delete;

  /// Virtual class instance is not movable.
  auto operator=(VirtualBase&&) -> VirtualBase& = delete;

  /// Virtual class instance is not copyable.
  auto operator=(const VirtualBase&) -> VirtualBase& = delete;

  /// Virtual destructor.
  virtual ~VirtualBase() = default;

}; // class VirtualBase

/// Is a virtual class instance of the given type?
template<std::derived_from<VirtualBase> Derived>
constexpr auto instance_of(const VirtualBase& instance) -> bool {
  return dynamic_cast<const Derived*>(&instance) != nullptr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
