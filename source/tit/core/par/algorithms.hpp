/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_reduce.h>
#include <oneapi/tbb/parallel_sort.h>
#include <oneapi/tbb/partitioner.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/par/atomic.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/par/partitioner.hpp" // IWYU pragma: export
#include "tit/core/utils.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Batch operations.
//

/// Iterate through the blocks of the range in parallel.
struct ForEachRange final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  /// @todo Clang segfaults if this or similar functions below are `static`.
  template<partitioner Partitioner,
           range Range,
           std::regular_invocable<blocked_range_t<Range>> Func>
  void operator()(Partitioner partitioner, Range&& range, Func func) const {
    TIT_ASSUME_UNIVERSAL(Range, range);
    tbb::parallel_for(partitioner.blockify(range), std::move(func));
  }
};

/// @copydoc ForEachRange
inline constexpr ForEachRange for_each_range{};

/// Iterate through the range in parallel.
struct ForEach final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  template<partitioner Partitioner,
           range Range,
           std::regular_invocable<std::ranges::range_reference_t<Range&&>> Func>
  void operator()(Partitioner partitioner, Range&& range, Func func) const {
    for_each_range(std::move(partitioner),
                   std::forward<Range>(range),
                   std::bind_back(std::ranges::for_each, std::move(func)));
  }
};

/// @copydoc ForEach
inline constexpr ForEach for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the blocks of the range in parallel.
/// Per-thread range partition will always be the same.
struct DeterministicForEachRange final {
  template<range Range,
           std::regular_invocable<blocked_range_t<Range>, size_t> Func>
  void operator()(Range&& range, Func func) const {
    TIT_ASSUME_UNIVERSAL(Range, range);
    const auto count = num_threads();
    const auto quotient = static_cast<size_t>(std::size(range)) / count;
    const auto remainder = static_cast<size_t>(std::size(range)) % count;
    const auto iter = std::begin(range);
    tbb::parallel_for(
        size_t{0},
        count,
        [quotient, remainder, iter, &func](size_t thread) {
          const std::ranges::subrange subrange{
              iter + thread * quotient + std::min(thread, remainder),
              iter + (thread + 1) * quotient + std::min(thread + 1, remainder)};
          std::invoke(func, subrange, thread);
        },
        tbb::static_partitioner{});
  }
};

/// @copydoc DeterministicForEachRange
inline constexpr DeterministicForEachRange deterministic_for_each_range{};

/// Iterate through the range in parallel.
/// All thread partition will always be the same.
struct DeterministicForEach final {
  template<range Range,
           std::regular_invocable<std::ranges::range_reference_t<Range&&>,
                                  size_t> Func>
  void operator()(Range&& range, Func func) const {
    deterministic_for_each_range(
        std::forward<Range>(range),
        [&func](std::ranges::view auto subrange, size_t thread) {
          std::ranges::for_each(std::move(subrange),
                                std::bind_back(std::ref(func), thread));
        });
  }
};

/// @copydoc DeterministicForEach
inline constexpr DeterministicForEach deterministic_for_each{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterate through the block of ranges in parallel.
struct BlockForEach final {
  template<range Range,
           std::regular_invocable<std::ranges::range_reference_t<
               std::ranges::range_reference_t<Range&&>>> Func>
  void operator()(Range&& range, Func func) const {
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

/// Parallel range-wise fold.
struct FoldRange final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  template<partitioner Partitioner,
           range Range,
           class Result = std::ranges::range_value_t<Range>,
           std::regular_invocable<blocked_range_t<Range>, Result> Func,
           std::regular_invocable<Result, Result> ResultFunc>
    requires (
        std::assignable_from<
            Result&,
            std::invoke_result_t<Func&, blocked_range_t<Range>, Result>> &&
        std::assignable_from<Result&,
                             std::invoke_result_t<ResultFunc&, Result, Result>>)
  auto operator()(Partitioner partitioner,
                  Range&& range,
                  Result init,
                  Func func,
                  ResultFunc result_func) const -> Result {
    TIT_ASSUME_UNIVERSAL(Range, range);
    return tbb::parallel_reduce(partitioner.blockify(range),
                                std::move(init),
                                std::move(func),
                                std::move(result_func));
  }
};

/// @copydoc FoldRange
inline constexpr FoldRange fold_range{};

/// Parallel fold.
struct Fold final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  template<partitioner Partitioner,
           range Range,
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
  auto operator()(Partitioner partitioner,
                  Range&& range,
                  Result init = {},
                  Func func = {},
                  ResultFunc result_func = {}) const -> Result {
    return fold_range(std::move(partitioner),
                      std::forward<Range>(range),
                      std::move(init), // NOLINTNEXTLINE(*-include-cleaner)
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
struct UnstableCopyIf final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  template<partitioner Partitioner,
           range Range,
           std::random_access_iterator OutIter,
           class Proj = std::identity,
           std::indirect_unary_predicate<
               std::projected<std::ranges::iterator_t<Range>, Proj>> Pred>
    requires std::indirectly_copyable<std::ranges::iterator_t<Range>, OutIter>
  auto operator()(Partitioner partitioner,
                  Range&& range,
                  OutIter out,
                  Pred pred,
                  Proj proj = {}) const -> OutIter {
    ssize_t index = 0;
    for_each_range(
        std::move(partitioner),
        std::forward<Range>(range),
        [&index, out, &pred, &proj](std::ranges::view auto subrange) {
          // Filter the chunk into the intermediate buffer, then move the buffer
          // into the output range. Intermediate buffer is used to reduce the
          // number of atomic operations.
          using Val = std::ranges::range_value_t<Range>;
          static constexpr size_t BufferCap = 64;
          InplaceVector<Val, BufferCap> buffer{};
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
struct Transform final : AutoPartitionerInvoker {
  using AutoPartitionerInvoker::operator();
  template<partitioner Partitioner,
           range Range,
           std::random_access_iterator OutIter,
           class Func,
           class Proj = std::identity>
    requires std::indirectly_writable<
        OutIter,
        std::indirect_result_t<
            Func&,
            std::projected<std::ranges::iterator_t<Range>, Proj>>>
  auto operator()(Partitioner partitioner,
                  Range&& range,
                  OutIter out,
                  Func func,
                  Proj proj = {}) const -> OutIter {
    const auto out_end = std::next(out, std::size(range));
    for_each(std::move(partitioner),
             std::views::zip(std::ranges::subrange{out, out_end},
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
  void operator()(Range&& range, Compare compare = {}, Proj proj = {}) const {
    TIT_ASSUME_UNIVERSAL(Range, range);
    using Ref = std::ranges::range_reference_t<Range>;
    tbb::parallel_sort(std::begin(range),
                       std::end(range),
                       [&compare, &proj](Ref& a, Ref& b) {
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
