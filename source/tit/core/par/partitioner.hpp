/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

#include <oneapi/tbb/blocked_range.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/par/control.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

// Mark the `tbb::blocked_range` as a view.
template<std::random_access_iterator Iter>
inline constexpr bool std::ranges::enable_view<tbb::blocked_range<Iter>> = true;

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range that could be processed in parallel.
template<class Range>
concept range =
    std::ranges::common_range<Range> && std::ranges::sized_range<Range> &&
    std::ranges::random_access_range<Range>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Blockified range.
template<range Range>
using blocked_range_t = tbb::blocked_range<std::ranges::iterator_t<Range>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Automatic parallelization partitioner.
struct AutoPartitioner final {
  /// Blockify the range.
  template<range Range>
  auto blockify(Range&& range) const noexcept -> blocked_range_t<Range&&> {
    TIT_ASSUME_UNIVERSAL(Range, range);
    return {std::begin(range), std::end(range)};
  }
};

/// @copydoc AutoPartitioner
inline constexpr AutoPartitioner auto_{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Static parallelization partitioner.
struct StaticPartitioner final {
  /// Blockify the range.
  template<range Range>
  auto blockify(Range&& range) const noexcept -> blocked_range_t<Range&&> {
    TIT_ASSUME_UNIVERSAL(Range, range);
    const auto grain_size_hint =
        divide_up(static_cast<size_t>(std::size(range)), num_threads());
    return {std::begin(range), std::end(range), grain_size_hint};
  }
};

/// @copydoc StaticPartitioner
inline constexpr StaticPartitioner static_{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partitioner type.
template<class P>
concept partitioner = std::same_as<P, AutoPartitioner> || //
                      std::same_as<P, StaticPartitioner>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Helper base class that provides `operator()` overload when no partitioner
/// is specified.
struct AutoPartitionerInvoker {
  /// Invoke the algorithm with the auto partitioner.
  template<std::derived_from<AutoPartitionerInvoker> Self,
           class Arg,
           class... RestArgs>
    requires (!partitioner<std::remove_cvref_t<Arg>>)
  auto operator()(this Self& self, Arg&& arg, RestArgs&&... rest_args) {
    return self(auto_,
                std::forward<Arg>(arg),
                std::forward<RestArgs>(rest_args)...);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
