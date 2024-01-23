/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm> // IWYU pragma: keep
#include <concepts>  // IWYU pragma: keep
#include <iterator>  // IWYU pragma: keep
#include <ranges>

#include <oneapi/tbb/global_control.h>

// #include "tit/core/misc.hpp"        // IWYU pragma: keep
#include "tit/core/trait_utils.hpp" // IWYU pragma: keep
#include "tit/core/types.hpp"

namespace tit::par {

// NOLINTBEGIN(*-missing-std-forward)

struct impl_tag_t {};
struct seg_tag_t : impl_tag_t {};
struct par_tag_t : impl_tag_t {};
inline constexpr seg_tag_t seg_tag{};
inline constexpr par_tag_t par_tag{};

struct sched_tag_t {};
struct static_tag_t : sched_tag_t {};
struct dynamic_tag_t : sched_tag_t {};
inline constexpr static_tag_t static_tag{};
inline constexpr dynamic_tag_t dynamic_tag{};

/** Wrapper for the `main` that sets up multithreading. */
template<std::invocable<int, char**> Func>
auto main(int argc, char** argv, Func&& func) -> int {
  tbb::global_control gc{tbb::global_control::max_allowed_parallelism, 8};
  return func(argc, argv);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if 0
/** Number of threads. */
constexpr auto num_threads() noexcept -> size_t {
  TIT_THREAD_FUNC_IMPL_(num_threads_);
}

/** Current thread index. */
constexpr auto thread_index() noexcept -> size_t {
  TIT_THREAD_FUNC_IMPL_(thread_index_);
}
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Range that could be processed in parallel. */
/** @{ */
template<class Range>
concept base_input_range =
    std::ranges::input_range<Range> &&
    std::ranges::random_access_range<Range> && std::ranges::sized_range<Range>;
template<class Range>
concept input_range =
    base_input_range<Range> ||
    (specialization_of<Range, std::ranges::join_view> &&
     base_input_range<decltype(std::declval<Range&&>().base())>);
/** @} */

template<class Func, class Range>
concept loop_body =
    base_input_range<Range> &&
    std::indirectly_regular_unary_invocable<Func,
                                            std::ranges::iterator_t<Range>>;

template<class Func, class BaseRange>
concept indirect_loop_body =
    base_input_range<BaseRange> &&
    std::indirectly_regular_unary_invocable<
        Func, std::ranges::iterator_t<std::ranges::range_value_t<BaseRange>>>;

#if 0
/** Iterate through the range in parallel (static partitioning). */
/** @{ */
template<base_input_range Range, loop_body<Range> Func>
constexpr void static_for_each(Range&& range, Func&& func) noexcept {
  TIT_THREAD_FUNC_IMPL_(for_each_, static_tag, range, func);
}
template<base_input_range BaseRange, indirect_loop_body<BaseRange> Func>
constexpr void static_for_each( //
    std::ranges::join_view<BaseRange> view, Func&& func) noexcept {
  static_for_each(std::move(view).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
/** @} */
#endif

template<class Range>
concept block_input_range = std::ranges::input_range<Range>;

template<block_input_range Range,
         std::indirectly_regular_unary_invocable<
             std::ranges::iterator_t<std::ranges::range_value_t<Range>>>
             Func>
constexpr void block_for_each(Range&& range, Func func) noexcept {
  // Split the range in chunks according to the number of threads and
  // walk though the chunks sequentially.
  const auto chucked_range =
      std::views::chunk(std::forward<Range>(range), num_threads());
  for (auto&& chuck : chucked_range) {
    // Subranges inside each chunk are supposed to be independent thus
    // are processed in parallel.
    for_each(chuck, [&](auto subrange) { //
      std::ranges::for_each(subrange, func);
    });
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTEND(*-missing-std-forward)

} // namespace tit::par

// #include "tit/par/thread_omp.hpp"
