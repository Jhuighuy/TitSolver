/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "tit/core/time_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Basic stopwatch.
\******************************************************************************/
class Profiler final {
public:

  /** Profiler is a static object. */
  Profiler() = delete;

  /** Enable profiling. Report will be printed at exit. */
  static void enable() noexcept;

  /** Stopwatch that is associated with a section. */
  static auto stopwatch(const std::string& section_name) -> Stopwatch&;

private:

  static void begin_profiling_();
  static void end_profiling_();

  // NOLINTBEGIN(*-avoid-non-const-global-variables)
  static std::mutex sections_mutex_;
  static std::unordered_map<std::string, Stopwatch> sections_;
  // NOLINTEND(*-avoid-non-const-global-variables)

}; // class Profiler

/** Profile the current scope. */
#define TIT_PROFILE_SECTION(section_name)                                      \
  /* NOLINTNEXTLINE(*-avoid-non-const-global-variables) */                     \
  static auto& prof_stopwatch = tit::Profiler::stopwatch(section_name);        \
  const auto prof_cycle = tit::StopwatchCycle{prof_stopwatch};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
