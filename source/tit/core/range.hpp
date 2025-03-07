/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/tuple.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Contiguous range with fixed size.
template<class Range>
concept contiguous_fixed_size_range =
    std::ranges::contiguous_range<Range> && std::ranges::sized_range<Range> &&
    decltype(std::span{std::declval<Range&>()})::extent != std::dynamic_extent;

/// Size of the contiguous fixed size range.
template<contiguous_fixed_size_range Range>
inline constexpr auto range_fixed_size_v =
    decltype(std::span{std::declval<Range&>()})::extent;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of ranges.
template<class Range>
concept multirange = std::ranges::input_range<Range> &&
                     std::ranges::range<std::ranges::range_reference_t<Range>>;

/// Range of input ranges.
template<class Range>
concept input_multirange =
    multirange<Range> &&
    std::ranges::input_range<std::ranges::range_reference_t<Range>>;

/// Value of a range of ranges.
template<class Range>
using multirange_value_t =
    std::ranges::range_value_t<std::ranges::range_reference_t<Range>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Input iterator with compatible value type.
template<class Iterator, class Val>
concept value_iterator = std::input_iterator<Iterator> &&
                         std::convertible_to<std::iter_value_t<Iterator>, Val>;

/// Input range with compatible value type.
template<class Range, class Val>
concept value_range =
    std::ranges::input_range<Range> &&
    std::convertible_to<std::ranges::range_value_t<Range>, Val>;

/// Input range of ranges with compatible value type.
template<class Range, class Val>
concept value_multirange = input_multirange<Range> &&
                           std::convertible_to<multirange_value_t<Range>, Val>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Input iterator with compatible tuple item types.
template<class Iterator, class... Items>
concept tuple_iterator = std::input_iterator<Iterator> &&
                         tuple_like<std::iter_value_t<Iterator>, Items...>;

/// Input range with convertible tuple item types.
template<class Range, class... Items>
concept tuple_range = std::ranges::input_range<Range> &&
                      tuple_like<std::ranges::range_value_t<Range>, Items...>;

/// Input range of ranges with convertible tuple item types.
template<class Range, class... Items>
concept tuple_multirange =
    input_multirange<Range> && tuple_like<multirange_value_t<Range>, Items...>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of indices.
template<class Indices>
concept index_range = std::ranges::random_access_range<Indices> &&
                      std::integral<std::ranges::range_value_t<Indices>>;

/// Range of indices that can be used as output for various algorithms.
template<class Indices>
concept output_index_range = std::ranges::random_access_range<Indices> &&
                             std::ranges::output_range<Indices, size_t>;

/// Input-output index range.
template<class Indices>
concept input_output_index_range =
    index_range<Indices> && output_index_range<Indices>;

/// Permuted range view.
template<std::ranges::random_access_range Range, index_range Perm>
  requires std::ranges::viewable_range<Range> &&
           std::ranges::viewable_range<Perm>
constexpr auto permuted_view(Range&& range, Perm&& perm) {
  TIT_ASSUME_UNIVERSAL(Range, range);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Find the equality ranges in the given range.
template<
    std::ranges::input_range Range,
    std::invocable<std::ranges::subrange<std::ranges::iterator_t<Range>>> Func,
    std::regular_invocable<std::ranges::range_reference_t<Range>> Proj =
        std::identity,
    std::indirect_binary_predicate<
        std::projected<std::ranges::iterator_t<Range>, Proj>,
        std::projected<std::ranges::iterator_t<Range>, Proj>> Pred =
        std::equal_to<>>
constexpr void equality_ranges(Range&& range,
                               Func func,
                               Pred pred = {},
                               Proj proj = {}) {
  TIT_ASSUME_UNIVERSAL(Range, range);
  auto iter = std::begin(range);
  const auto last = std::end(range);
  while (iter != last) {
    const auto next = std::ranges::find_if_not( //
        iter,
        last,
        std::bind_front(std::ref(pred), std::invoke(proj, *iter)),
        proj);
    std::invoke(func, std::ranges::subrange{iter, next});
    iter = next;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a value to a byte array.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_byte_array(const T& value) -> std::array<byte_t, sizeof(T)> {
  return std::bit_cast<std::array<byte_t, sizeof(T)>>(value);
}

/// Convert a value to a byte vector.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto to_bytes(const T& value) -> std::vector<byte_t> {
  const auto bytes_array = to_byte_array(value);
  return {bytes_array.begin(), bytes_array.end()};
}

/// Convert a byte array to a value.
template<class T>
  requires std::is_trivially_copyable_v<T>
constexpr auto from_bytes(std::span<const byte_t> bytes) -> T {
  TIT_ASSERT(bytes.size() >= sizeof(T), "Invalid byte array size!");
  std::array<byte_t, sizeof(T)> bytes_array{};
  std::ranges::copy(bytes, bytes_array.begin());
  return std::bit_cast<T>(bytes_array);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
