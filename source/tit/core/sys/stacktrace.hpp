/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <ranges>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace/stacktrace.hpp>

#include "tit/core/missing.hpp" // IWYU pragma: keep

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stack trace.
class Stacktrace final : public boost::stacktrace::stacktrace {
public:

  using boost::stacktrace::stacktrace::stacktrace;

  /// Retrieve the current stack trace.
  [[gnu::always_inline]] static auto current() noexcept -> Stacktrace {
    return {};
  }

}; // class Stacktrace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Stack trace formatter.
template<>
struct std::formatter<tit::Stacktrace> {
  constexpr auto parse(auto& context) {
    return context.begin();
  }
  constexpr auto format(const tit::Stacktrace& stacktrace,
                        auto& context) const {
    auto out = context.out();
    out = std::format_to(out, "\nStack trace:\n");
    for (const auto& [index, frame] : std::views::enumerate(stacktrace)) {
      out = std::format_to(out,
                           "\n{:>3} {} {}",
                           index,
                           frame.address(),
                           frame.name());
    }
    return out;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
