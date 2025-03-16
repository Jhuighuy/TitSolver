/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
  static constexpr auto parse(const std::format_parse_context& context) {
    return context.begin();
  }
  static constexpr auto format(const tit::Stacktrace& stacktrace,
                               std::format_context& context) {
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
