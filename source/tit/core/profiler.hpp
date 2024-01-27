/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "tit/core/misc.hpp"
#include "tit/core/time_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Profiler interface.
\******************************************************************************/
class Profiler final {
public:

  /** Profiler is a static object. */
  Profiler() = delete;

  /** Stopwatch that is associated with a section. */
  static auto section(std::string_view section_name) -> Stopwatch&;

  /** Enable profiling. Report will be printed at exit. */
  static void enable() noexcept;

private:

  static std::mutex sections_mutex_;
  static std::unordered_map<std::string, Stopwatch> sections_;

}; // class Profiler

/** Profile the current scope. */
#define TIT_PROFILE_SECTION(section_name)                                      \
  static auto& TIT_NAME(prof_section) = tit::Profiler::section(section_name);  \
  tit::StopwatchCycle const TIT_NAME(prof_cycle){TIT_NAME(prof_section)};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
