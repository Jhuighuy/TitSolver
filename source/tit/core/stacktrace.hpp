/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <ranges>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace/stacktrace.hpp>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stack trace.
class Stacktrace final : public boost::stacktrace::stacktrace {
public:

  /// Get the current stack trace.
  static auto current() {
    return Stacktrace{};
  }

private:

  Stacktrace() = default;

}; // class Stacktrace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

template<>
struct std::formatter<tit::Stacktrace> {
  static constexpr auto parse(const std::format_parse_context& context) {
    return context.begin();
  }

  static auto format(const tit::Stacktrace& stacktrace,
                     std::format_context& context) {
    auto out = context.out();

    for (const auto& [index, frame] : std::views::enumerate(stacktrace)) {
      out = std::format_to(out,
                           "{:>3} {} {}\n",
                           index,
                           frame.address(),
                           frame.name());
    }

    return out;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
