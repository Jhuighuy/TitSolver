/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <string_view>

#include "tit/core/str_utils.hpp"
#include "tit/core/time.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Profiler interface.
class Profiler final {
public:

  /// Profiler is a static object.
  Profiler() = delete;

  /// Stopwatch associated with a section.
  static auto section(std::string_view section_name) -> Stopwatch&;

  /// Enable profiling. Report will be printed at exit.
  static void enable() noexcept;

private:

  static void report_();

  static StrHashMap<Stopwatch> sections_;

}; // class Profiler

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Profile the current scope.
#define TIT_PROFILE_SECTION(section_name)                                      \
  static auto& TIT_NAME(prof_section) = tit::Profiler::section(section_name);  \
  const tit::StopwatchCycle TIT_NAME(prof_cycle)(TIT_NAME(prof_section))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
