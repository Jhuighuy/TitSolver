/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>

#include <omp.h>

#include "tit/core/config.hpp"
#include "tit/core/types.hpp"
#include "tit/par/thread.hpp"

namespace tit::par {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<>
class Threading<par_tag_t> {
public:

  template<std::invocable Func>
  static auto main_([[maybe_unused]] int argc, [[maybe_unused]] char** argv,
                    Func&& func) -> int {
    return func();
  }

  static constexpr auto num_threads_() noexcept -> size_t {
    return 1;
  }
  static constexpr auto thread_index_() noexcept -> size_t {
    return 0;
  }

  template<std::invocable... Funcs>
  static constexpr void invoke_(Funcs&&... funcs) noexcept {
    (funcs(), ...);
  }

  template<std::ranges::input_range Range,
           std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Func>
  static constexpr void for_each_([[maybe_unused]] sched_tag_t shed_tag, //
                                  Range&& range, Func&& func) noexcept {
    std::ranges::for_each(range, func);
  }

}; // class Threading

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto num_threads() noexcept -> size_t {
  // return static_cast<size_t>(omp_get_max_threads());
  return 4;
}

auto thread_index() noexcept -> size_t {
  return static_cast<size_t>(omp_get_thread_num());
}

template<std::invocable Func>
auto main(int argc, char** argv, Func&& func) noexcept -> int {
  omp_set_num_threads(4);
  return func();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<std::invocable... Funcs>
void invoke(Funcs&&... funcs) noexcept {
  (funcs(), ...);
}

template<class Range, class Func>
void for_each(Range&& range, Func&& func,
              [[maybe_unused]] size_t grain_size = 100) noexcept {
  const auto end = std::ranges::end(range);
#pragma omp parallel for schedule(dynamic, grain_size)
  for (auto iter = std::ranges::begin(range); iter != end; ++iter) func(*iter);
}
template<class Range, class Func>
constexpr void for_each(std::ranges::join_view<Range> range, Func&& func,
                        size_t grain_size = 100) noexcept {
  for_each(
      std::move(range).base(),
      [&](auto subrange) { std::ranges::for_each(subrange, func); },
      grain_size);
}

template<class Range, class Func>
constexpr void static_for_each(Range&& range, Func&& func) noexcept {
  const auto end = std::ranges::end(range);
#pragma omp parallel for schedule(static)
  for (auto iter = std::ranges::begin(range); iter != end; ++iter) func(*iter);
}
template<class Range, class Func>
constexpr void static_for_each(std::ranges::join_view<Range> range,
                               Func&& func) noexcept {
  static_for_each(std::move(range).base(), [&](auto subrange) {
    std::ranges::for_each(subrange, func);
  });
}

template<
    std::ranges::input_range Range,
    class /*std::indirectly_unary_invocable<std::ranges::iterator_t<Range>>*/
    Func>
constexpr void block_for_each(Range&& range, Func&& func) noexcept {
#pragma omp parallel
  { std::ranges::for_each(range[thread_index()], func); }
  std::ranges::for_each(range[4], func);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::par
