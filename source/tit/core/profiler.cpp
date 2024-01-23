/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstdio>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tit/core/checks.hpp"
#include "tit/core/io_utils.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/system_utils.hpp"
#include "tit/core/time_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::mutex Profiler::sections_mutex_{};
std::unordered_map<std::string, Stopwatch> Profiler::sections_{};

auto Profiler::section(std::string_view section_name) -> Stopwatch& {
  TIT_ASSERT(!section_name.empty(), "Section name must not be empty!");
  const std::scoped_lock lock{sections_mutex_};
  return sections_[std::string{section_name}];
}

void Profiler::enable() noexcept {
  // Start profiling.
  constexpr const auto* root_section_name = "main";
  sections_[root_section_name].start();
  safe_atexit([] {
    // Stop profiling.
    sections_[root_section_name].stop();
    // Print the report.
    /// Presort the sections by total time.
    using SectionPtr = typename decltype(sections_)::const_pointer;
    std::vector<SectionPtr> sorted_sections{};
    sorted_sections.reserve(sections_.size());
    for (const auto& section : sections_) sorted_sections.push_back(&section);
    std::ranges::sort(sorted_sections, std::greater{},
                      [](SectionPtr s) { return s->second.total_ns(); });
    /// Print the sections table. At some point of time, we may want to replace
    /// this hardcoded table printing with a proper utility function.
    const auto width = tty_width(stdout);
    constexpr std::string_view abs_time_row = "abs. time [s]";
    constexpr std::string_view rel_time_row = "rel. time [%]";
    constexpr std::string_view calls_row = "calls [#]";
    constexpr std::string_view section_row = "section name";
    print("\nProfiling report:\n\n");
    println("{:->{}}", "", width);
    println("{}    {}    {}    {}", //
            abs_time_row, rel_time_row, calls_row, section_row);
    println("{:->{}}", "", width);
    const auto root_absolute_time = sorted_sections.front()->second.total();
    for (const SectionPtr section : sorted_sections) {
      const auto& [section_name, stopwatch] = *section;
      const auto abs_time = stopwatch.total();
      const auto rel_time = 100.0 * abs_time / root_absolute_time;
      const auto calls = stopwatch.cycles();
      println("{:>{}.5f}    {:>{}.5f}    {:>{}}    {}", //
              abs_time, abs_time_row.size(), rel_time, rel_time_row.size(),
              calls, calls_row.size(), section_name);
    }
    print("{:->{}}\n\n", "", width);
  });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
