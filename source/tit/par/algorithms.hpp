/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <boost/container/static_vector.hpp>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_reduce.h>
#include <oneapi/tbb/parallel_sort.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/utils.hpp"
#include "tit/par/atomic.hpp"
#include "tit/par/control.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Range and partitioner.
//

/// Range that could be processed in parallel.
template<class Range>
concept range =
    std::ranges::common_range<Range> && std::ranges::sized_range<Range> &&
    std::ranges::random_access_range<Range>;

/// Blockified range.
template<range Range>
using blocked_range_t = tbb::blocked_range<std::ranges::iterator_t<Range>>;

namespace impl {

template<range Range>
static auto make_blocked(Range&& range) noexcept -> blocked_range_t<Range&&> {
  TIT_ASSUME_UNIVERSAL(Range, range);
  return {std::begin(range), std::end(range)};
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Batch operations.
//

/// Iterate through the blocks of the range in parallel.
struct ForEachRange final {
  template<range Range, std::regular_invocable<blocked_range_t<Range>> Func>
  static void operator()(Range&& range, Func func) {
    tbb::parallel_for(impl::make_blocked(std::forward<Range>(range)),
                      std::move(func));
  }
};

/// @copydoc ForEachRange
inline constexpr ForEachRange for_each_range{};

/// Iterate through the range in parallel.
struct ForEach final {
  template<range Range,
           std::regular_invocable<std::ranges::range_reference_t<Range&&>> Func>
  static void operator()(Range&& range, Func func) {
    for_each_range(std::forward<Range>(range),
                   std::bind_back(std::ranges::for_each, std::move(func)));
  }
};

/// @copydoc ForEach
inline constexpr ForEach for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the block of ranges in parallel.
struct BlockForEach final {
  template<range Range,
           std::regular_invocable<std::ranges::range_reference_t<
               std::ranges::range_reference_t<Range&&>>> Func>
  static void operator()(Range&& range, Func func) {
    std::ranges::for_each(
        std::views::chunk(std::forward<Range>(range), num_threads()),
        std::bind_back(for_each,
                       std::bind_back(std::ranges::for_each, std::ref(func))));
  }
};

/// @copydoc BlockForEach
inline constexpr BlockForEach block_for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Fold operations.
//

/// Parallel fold.
struct Fold final {
  template<range Range,
           class Result = std::ranges::range_value_t<Range>,
           std::regular_invocable<Result, std::ranges::range_reference_t<Range>>
               Func = std::plus<>,
           std::regular_invocable<Result, Result> ResultFunc = Func>
    requires (
        std::assignable_from<
            Result&,
            std::invoke_result_t<Func&,
                                 Result,
                                 std::ranges::range_reference_t<Range>>> &&
        std::assignable_from<Result&,
                             std::invoke_result_t<ResultFunc&, Result, Result>>)
  static auto operator()(Range&& range,
                         Result init = {},
                         Func func = {},
                         ResultFunc result_func = {}) -> Result {
    return tbb::parallel_reduce(
        impl::make_blocked(std::forward<Range>(range)),
        std::move(init),
        std::bind_back(std::ranges::fold_left, std::move(func)),
        std::move(result_func));
  }
};

/// @copydoc Fold
inline constexpr Fold fold{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copy operations.
//

/// Parallel unstable copy-if.
/// Relative order of the elements in the output range is not preserved.
struct UnstableCopyIf final {
  template<range Range,
           std::random_access_iterator OutIter,
           class Proj = std::identity,
           std::indirect_unary_predicate<
               std::projected<std::ranges::iterator_t<Range>, Proj>> Pred>
    requires std::indirectly_copyable<std::ranges::iterator_t<Range>, OutIter>
  static auto operator()(Range&& range, OutIter out, Pred pred, Proj proj = {})
      -> OutIter {
    ssize_t index = 0;
    for_each_range(
        std::forward<Range>(range),
        [&index, out, &pred, &proj](std::ranges::view auto subrange) {
          // Filter the chunk into the intermediate buffer, then move the buffer
          // into the output range. Intermediate buffer is used to reduce the
          // number of atomic operations.
          using Val = std::ranges::range_value_t<Range>;
          static constexpr size_t BufferCap = 64;
          boost::container::static_vector<Val, BufferCap> buffer{};
          for (const auto& chunk :
               std::views::chunk(std::move(subrange), BufferCap)) {
            std::ranges::copy_if(chunk,
                                 std::back_inserter(buffer),
                                 std::ref(pred),
                                 std::ref(proj));
            std::ranges::move(buffer,
                              out + fetch_and_add(index, buffer.size()));
            buffer.clear();
          }
        });
    return out + index;
  }
};

/// @copydoc CopyIf
inline constexpr UnstableCopyIf unstable_copy_if{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Transformation operations.
//

/// Parallel transform.
struct Transform final {
  template<range Range,
           std::random_access_iterator OutIter,
           class Func,
           class Proj = std::identity>
    requires std::indirectly_writable<
        OutIter,
        std::indirect_result_t<
            Func&,
            std::projected<std::ranges::iterator_t<Range>, Proj>>>
  static auto operator()(Range&& range, OutIter out, Func func, Proj proj = {})
      -> OutIter {
    const auto out_end = std::next(out, std::size(range));
    for_each(std::views::zip(std::ranges::subrange{out, out_end},
                             std::views::as_const(std::forward<Range>(range))),
             [&func, &proj](auto arg) {
               std::get<0>(arg) =
                   std::invoke(func, std::invoke(proj, std::get<1>(arg)));
             });
    return out_end;
  }
};

/// @copydoc Transform
inline constexpr Transform transform{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Sorting operations.
//

/// Parallel sort.
struct Sort final {
  template<range Range, class Compare = std::less<>, class Proj = std::identity>
    requires std::sortable<std::ranges::iterator_t<Range>, Compare, Proj>
  static void operator()(Range&& range, Compare compare = {}, Proj proj = {}) {
    TIT_ASSUME_UNIVERSAL(Range, range);
    tbb::parallel_sort(
        std::begin(range),
        std::end(range),
        [&compare, &proj](std::ranges::range_reference_t<Range> a,
                          std::ranges::range_reference_t<Range> b) {
          return std::invoke(compare,
                             std::invoke(proj, a),
                             std::invoke(proj, b));
        });
  }
};

/// @copydoc Sort
inline constexpr Sort sort{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par

template<std::random_access_iterator Iter>
inline constexpr bool std::ranges::enable_view<tbb::blocked_range<Iter>> = true;
