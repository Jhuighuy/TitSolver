/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/compat.hpp"
#include "tit/core/posix_utils.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/time_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTBEGIN(*-avoid-non-const-global-variables)
std::mutex Profiler::sections_mutex_{};
std::unordered_map<std::string, Stopwatch> Profiler::sections_{};
// NOLINTEND(*-avoid-non-const-global-variables)

void Profiler::enable() noexcept {
  begin_profiling_();
  const auto status = std::atexit(&end_profiling_);
  TIT_ENSURE(status == 0, "Unable register the profiler reporter!");
}

auto Profiler::stopwatch(const std::string& section_name) -> Stopwatch& {
  TIT_ASSERT(!section_name.empty(), "Section name must not be empty!");
  const std::scoped_lock lock{sections_mutex_};
  return sections_[section_name];
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

inline constexpr const auto* root_section_name = "main";

void Profiler::begin_profiling_() {
  sections_[root_section_name].start();
}

void Profiler::end_profiling_() {
  sections_[root_section_name].stop();
  // Print the report.
  /// Presort the sections by total time.
  using SectionPtr = typename decltype(sections_)::const_pointer;
  std::vector<SectionPtr> sorted_sections{};
  sorted_sections.reserve(sections_.size());
  for (const auto& section : sections_) sorted_sections.push_back(&section);
  std::ranges::sort(sorted_sections, std::greater{},
                    [](SectionPtr s) { return s->second.total(); });
  /// Print the report.
  const auto separator = std::string(terminal_width(stdout), '-');
  Std::println("\nProfiling report:\n");
  Std::println("{}", separator);
  Std::println(" abs. time (s)    rel. time (%)    calls (#)    section name");
  Std::println("{}", separator);
  const auto root_absolute_time = sorted_sections.front()->second.total();
  for (const SectionPtr section : sorted_sections) {
    const auto& [name, stopwatch] = *section;
    const auto absolute_time = stopwatch.total();
    const auto relative_time = 100.0 * absolute_time / root_absolute_time;
    Std::println(" {:>13.5f}    {:>13.5f}    {:>9}    {}", //
                 absolute_time, relative_time, stopwatch.cycles(), name);
  }
  Std::println("{}", separator);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
