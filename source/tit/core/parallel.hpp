/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts> // IWYU pragma: keep
#include <functional>
#include <ranges> // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/partitioner.h>

// #include "tit/core/misc.hpp"
#include "tit/core/compat.hpp"
#include "tit/core/trait_utils.hpp" // IWYU pragma: keep
#include "tit/core/types.hpp"

namespace tit::par {

constexpr auto num_threads() noexcept -> size_t {
  return 8;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Invoke functions in parallel and wait for all of them to complete. */
class Invoke {
public:

  template<class... Funcs>
    requires (std::invocable<Funcs> && ...) &&
             (std::same_as<std::invoke_result_t<Funcs>, void> && ...)
  void operator()(Funcs... funcs) const noexcept {
    tbb::parallel_invoke(std::move(funcs)...);
  }

}; // class Invoke

/** @copydoc Invoke */
inline constexpr Invoke invoke{};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Underlying range type. */
template<std::ranges::view View>
using view_base_t = decltype(std::declval<View>().base());

/** Simple range that could be processed in parallel. */
template<class Range>
concept simple_range =
    std::ranges::sized_range<Range> && std::ranges::random_access_range<Range>;

/** Range that could be processed in parallel. */
template<class Range>
concept range =
    simple_range<Range> || (specialization_of<Range, std::ranges::join_view> &&
                            simple_range<view_base_t<Range>>);

/** Iterate through the range in parallel (dynamic partitioning). */
class ForEach final {
public:

  /** @{ */
  template<simple_range Range, class Func>
  void operator()(Range&& range, Func func) const noexcept {
    const auto view = std::views::all(std::forward<Range>(range));
    tbb::parallel_for(tbb::blocked_range{view.begin(), view.end()},
                      Std::bind_back(std::ranges::for_each, std::move(func)));
  }
  template<simple_range Range, class Func>
  void operator()(std::ranges::join_view<Range> join_view,
                  Func func) const noexcept {
    (*this)(std::move(join_view).base(),
            Std::bind_back(std::ranges::for_each, std::move(func)));
  }
  /** @} */

}; // class ForEach

/** @copydoc ForEach */
inline constexpr ForEach for_each{};

/** Iterate through the good_range in parallel (static partitioning). */
class StaticForEach final {
public:

  template<std::invocable<size_t> Func>
  void operator()(Func func) const noexcept {
    tbb::parallel_for<size_t>(
        /*first=*/0, /*last=*/num_threads(), /*step=*/1, //
        std::move(func), tbb::static_partitioner{});
  }

  /** @{ */
  template<simple_range Range, class Func>
  void operator()(Range&& range, Func func) const noexcept {
    auto view = std::views::all(std::forward<Range>(range));
    const auto size = std::ranges::size(view);
    auto begin_block = [quotient = size / num_threads(),
                        remainder = size % num_threads(),
                        view = std::move(view)](size_t index) {
      const auto offset = index * quotient + std::min(index, remainder);
      return view.begin() + offset;
    };
    (*this)([begin_block = std::move(begin_block),
             func = std::move(func)](size_t thread_index) {
      std::for_each(begin_block(thread_index), begin_block(thread_index + 1),
                    std::bind_front(std::move(func), thread_index));
    });
  }
  template<simple_range Range, class Func>
  void operator()(std::ranges::join_view<Range> join_view,
                  Func func) const noexcept {
    (*this)(
        std::move(join_view).base(),
        [func = std::move(func)](size_t thread_index, const auto& subrange) {
          std::ranges::for_each(subrange,
                                std::bind_front(std::move(func), thread_index));
        });
  }
  /** @} */

}; // class StaticForEach

/** @copydoc StaticForEach */
inline constexpr StaticForEach static_for_each{};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par

#include "tit/par/thread.hpp" // IWYU pragma: exports
