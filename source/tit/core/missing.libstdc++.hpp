/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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

#include "tit/core/utils.hpp"

// NOLINTBEGIN(cert-dcl58-cpp)
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
constexpr auto format(const auto& /*fmt*/, Range&& range) -> std::string {
  TIT_ASSUME_UNIVERSAL(Range, range);
  if (std::ranges::empty(range)) return "[]";
  std::string result = std::format("[{}", *std::begin(range));
  for (const auto& elem : range | std::views::drop(1)) {
    result += std::format(", {}", elem);
  }
  result += "]";
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)
// NOLINTEND(cert-dcl58-cpp)

#endif // ifdef __GLIBCXX__
