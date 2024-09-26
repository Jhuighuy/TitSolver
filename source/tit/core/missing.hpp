/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <functional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace/stacktrace.hpp>

#include "tit/core/utils.hpp"

namespace tit::Std {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Simple implementation of `std::bind_back`, since Clang 18 cannot handle
/// it's libstdc++ implementation.
///
/// @todo To be removed when Clang 19 is released.
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Format a range.
///
/// @todo To be removed when ranges are supported in `std::format`.
template<std::ranges::input_range Range>
  requires std::formattable<std::ranges::range_value_t<Range>, char>
constexpr auto format_range(Range&& range) -> std::string {
  TIT_ASSUME_UNIVERSAL(Range, range);
  if (std::empty(range)) return "[]";
  std::string result = std::format("[{}", *std::begin(range));
  for (const auto& elem : range | std::views::drop(1)) {
    result += std::format(", {}", elem);
  }
  result += "]";
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stack trace.
///
/// @todo To be removed when we have `std::stacktrace`.
class stacktrace final : public boost::stacktrace::stacktrace {
public:

  using boost::stacktrace::stacktrace::stacktrace;

  /// Retrieve the current stack trace.
  [[gnu::always_inline]]
  static auto current() noexcept -> stacktrace {
    return {};
  }

}; // class stacktrace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::Std
