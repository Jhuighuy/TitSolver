/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/checks.hpp"
#include "tit/core/print.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/time.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

StrHashMap<Stopwatch> Profiler::sections_{};

auto Profiler::section(std::string_view section_name) -> Stopwatch& {
  TIT_ASSERT(!section_name.empty(), "Section name must not be empty!");
  /// @todo In C++26 there would be no need for `std::string{...}`.
  return sections_[std::string{section_name}];
}

void Profiler::enable() noexcept {
  // Start profiling.
  constexpr const auto* root_section_name = "main";
  section(root_section_name).start();

  // Stop profiling and report at exit.
  checked_atexit([] {
    section(root_section_name).stop();
    report_();
  });
}

void Profiler::report_() {
  // Gather the sections and sort them by total time.
  auto sorted_sections =
      sections_ |
      std::views::transform([](auto& section) { return &section; }) |
      std::ranges::to<std::vector>();
  std::ranges::sort(sorted_sections, std::greater{}, [](const auto* s) {
    return s->second.total_ns();
  });

  // Print the report table.
  const auto width = tty_width(TTY::Stdout).value_or(80);
  constexpr std::string_view abs_time_title = "abs. time [s]";
  constexpr std::string_view rel_time_title = "rel. time [%]";
  constexpr std::string_view num_calls_title = "calls [#]";
  constexpr std::string_view section_title = "section name";
  println();
  println("Profiling report:");
  println();
  println("{:->{}}", "", width);
  println("{}    {}    {}    {}",
          abs_time_title,
          rel_time_title,
          num_calls_title,
          section_title);
  println("{:->{}}", "", width);
  const auto root_absolute_time = sorted_sections.front()->second.total();
  for (const auto* section : sorted_sections) {
    const auto& [section_name, stopwatch] = *section;
    const auto abs_time = stopwatch.total();
    const auto rel_time = 100.0 * abs_time / root_absolute_time;
    const auto num_calls = stopwatch.cycles();
    println("{:>{}.5f}    {:>{}.5f}    {:>{}}    {}",
            abs_time,
            abs_time_title.size(),
            rel_time,
            rel_time_title.size(),
            num_calls,
            num_calls_title.size(),
            section_name);
  }
  println("{:->{}}", "", width);
  println();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
