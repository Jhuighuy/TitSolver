/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm> // IWYU pragma: keep
#include <concepts>
#include <iterator>
#include <ranges>

#include <omp.h>

#include "tit/core/config.hpp"
#include "tit/core/misc.hpp"
#include "tit/core/types.hpp"

namespace tit::par {
// NOLINTBEGIN(cppcoreguidelines-missing-std-forward)

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

template<class Tag>
class Threading {
public:

  template<std::invocable Func>
  static auto main_([[maybe_unused]] int argc, [[maybe_unused]] char** argv,
                    Func&& func) -> int {
    omp_set_num_threads(8);
    return func();
    // return func();
  }

  static constexpr auto num_threads_() noexcept -> size_t {
    return 8;
    // return 1;
  }
  static constexpr auto thread_index_() noexcept -> size_t {
    return static_cast<size_t>(omp_get_thread_num());
    // return 0;
  }

  template<std::invocable... Funcs>
  static constexpr void invoke_(Funcs&&... funcs) noexcept {
    (funcs(), ...);
  }

  template<class Range, class Func>
  static void for_each_(dynamic_tag_t /**/, Range&& range, Func&& func,
                        [[maybe_unused]] size_t grain_size = 100) noexcept {
    const auto end = std::ranges::end(range);
#pragma omp parallel for schedule(dynamic, grain_size)
    for (auto iter = std::ranges::begin(range); iter != end; ++iter)
      func(*iter);
  }

  template<class Range, class Func>
  static void for_each_(static_tag_t /**/, Range&& range,
                        Func&& func) noexcept {
    const auto end = std::ranges::end(range);
#pragma omp parallel for schedule(static)
    for (auto iter = std::ranges::begin(range); iter != end; ++iter)
      func(*iter);
  }

  // template<std::ranges::input_range Range,
  //          std::indirectly_unary_invocable<std::ranges::iterator_t<Range>>
  //          Func>
  // static constexpr void for_each_([[maybe_unused]] sched_tag_t shed_tag, //
  //                                 Range&& range, Func&& func) noexcept {
  //   std::ranges::for_each(range, func);
  // }

}; // class Threading

// Generate a constexpr-aware overload for a threading function.
#define TIT_THREAD_FUNC_IMPL_(func, ...)                                       \
  if consteval {                                                               \
    return Threading<seg_tag_t>::func(__VA_ARGS__);                            \
  } else { /* NOLINT(readability-else-after-return) */                         \
    return Threading<par_tag_t>::func(__VA_ARGS__);                            \
  }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Wrapper for the `main` that sets up multithreading. */
template<std::invocable Func>
auto main(int argc, char** argv, Func&& func) -> int {
  TIT_THREAD_FUNC_IMPL_(main_, argc, argv, func);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Number of threads. */
constexpr auto num_threads() noexcept -> size_t {
  TIT_THREAD_FUNC_IMPL_(num_threads_);
}

/** Current thread index. */
constexpr auto thread_index() noexcept -> size_t {
  TIT_THREAD_FUNC_IMPL_(thread_index_);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Invoke functions in parallel. */
template<std::invocable... Funcs>
constexpr void invoke(Funcs&&... funcs) noexcept {
  TIT_THREAD_FUNC_IMPL_(invoke_, funcs...);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Range that could be precessed in parallel. */
/** @{ */
template<class Range>
concept base_input_range =
    std::ranges::input_range<Range> &&
    std::ranges::random_access_range<Range> && std::ranges::sized_range<Range>;
template<class Range>
concept input_range =
    base_input_range<Range>
#if !TIT_LIBCPP // libc++ has no `std::ranges::join_view` yet.
    || (specialization_of<Range, std::ranges::join_view> &&
        base_input_range<decltype(std::declval<Range&&>().base())>)
#endif
    ;
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

/** Iterate through the range in parallel (dynamic partitioning). */
/** @{ */
template<base_input_range Range, loop_body<Range> Func>
constexpr void for_each(Range&& range, Func&& func) noexcept {
  TIT_THREAD_FUNC_IMPL_(for_each_, dynamic_tag, range, func);
}
#if !TIT_LIBCPP // libc++ has no `std::ranges::join_view` yet.
template<base_input_range BaseRange, indirect_loop_body<BaseRange> Func>
constexpr void for_each( //
    std::ranges::join_view<BaseRange> view, Func&& func) noexcept {
  for_each(std::move(view).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func); //
  });
}
#endif
/** @} */

/** Iterate through the range in parallel (static partitioning). */
/** @{ */
template<base_input_range Range, loop_body<Range> Func>
constexpr void static_for_each(Range&& range, Func&& func) noexcept {
  TIT_THREAD_FUNC_IMPL_(for_each_, static_tag, range, func);
}
#if !TIT_LIBCPP // libc++ has no `std::ranges::join_view` yet.
template<base_input_range BaseRange, indirect_loop_body<BaseRange> Func>
constexpr void static_for_each( //
    std::ranges::join_view<BaseRange> view, Func&& func) noexcept {
  static_for_each(std::move(view).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}
#endif
/** @} */

template<class Range>
concept block_input_range = std::ranges::input_range<Range>;

template<block_input_range Range,
         std::indirectly_regular_unary_invocable<
             std::ranges::iterator_t<std::ranges::range_value_t<Range>>>
             Func>
constexpr void block_for_each(Range&& range, Func&& func) noexcept {
#if TIT_LIBCPP
  // libc++ has no `std::views::chunk` yet.
  assume_used(range, func);
#else
  // Split the range in chunks according to the number of threads and
  // walk though the chunks sequentially.
  const auto chucked_range =
      std::views::chunk(std::forward<Range>(range), num_threads());
  for (auto&& chuck : chucked_range) {
    // Subranges inside each chunk are supposed to be independent thus
    // are processed in parallel.
    for_each(chuck, [&](auto subrange) {
      std::ranges::for_each(subrange, func); //
    });
  }
#endif
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTEND(cppcoreguidelines-missing-std-forward)
} // namespace tit::par

// #include "tit/par/thread_omp.hpp"
