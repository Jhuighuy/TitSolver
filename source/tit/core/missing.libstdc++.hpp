/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef> // IWYU pragma: keep
#ifdef __GLIBCXX__

#include <bits/c++config.h>
#include <concepts>
#include <format>
#include <functional>
#include <ranges>
#include <string>
#include <tuple>

namespace std _GLIBCXX_VISIBILITY(default) {
_GLIBCXX_BEGIN_NAMESPACE_VERSION

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __clang__
// Simple implementation of `std::bind_back`, since Clang 18 cannot handle
// the libstdc++ implementation.
template<class Func, class... BackArgs>
constexpr auto bind_back(Func&& func, BackArgs&&... back_arguments) {
  return
      [func = std::forward<Func>(func),
       back_args_tuple = std::tuple{std::forward<BackArgs>(
           back_arguments)...}]<class... FrontArgs>(FrontArgs&&... front_args) {
        return std::apply(
            [&func, &front_args...](auto&... back_args) {
              return std::invoke(func,
                                 std::forward<FrontArgs>(front_args)...,
                                 back_args...);
            },
            back_args_tuple);
      };
}
#endif

// Simple range formatter.
template<std::ranges::input_range Range>
  requires (std::formattable<std::ranges::range_value_t<Range>, char> &&
            !std::constructible_from<std::string, Range &&>)
struct formatter<Range> {
  constexpr auto parse(auto& context) {
    return context.begin();
  }
  constexpr auto format(const Range& range, auto& context) const {
    auto out = context.out();
    if (std::ranges::empty(range)) return std::format_to(out, "[]");
    out = std::format_to(out, "[{}", *std::begin(range));
    for (const auto& elem : range | std::views::drop(1)) {
      out = std::format_to(out, ", {}", elem);
    }
    return std::format_to(out, "]");
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)

#endif // ifdef __GLIBCXX__
