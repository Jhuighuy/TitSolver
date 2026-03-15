/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __linux__

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/posix.hpp"
#include "tit/core/proc_info.hpp"
#include "tit/core/sys_info.hpp"

namespace tit::proc_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto to_ns(uint64_t ticks, uint64_t clock_ticks_per_second) -> uint64_t {
  return static_cast<uint64_t>(
      std::llround(1.0e9L * static_cast<long double>(ticks) /
                   static_cast<long double>(clock_ticks_per_second)));
}

} // namespace

auto query_usage(pid_t pid) -> UsageSnapshot {
  TIT_ASSERT(pid > 0, "Invalid process ID!");

  const auto stat_path =
      std::filesystem::path{"/proc"} / std::to_string(pid) / "stat";
  std::ifstream stat_stream{stat_path};
  TIT_ENSURE(stat_stream.is_open(), "Unable to open '{}'.", stat_path.string());

  std::string stat_line;
  std::getline(stat_stream, stat_line);
  TIT_ENSURE(!stat_line.empty(), "Unable to read '{}'.", stat_path.string());

  const auto comm_end = stat_line.rfind(')');
  TIT_ENSURE(comm_end != std::string::npos,
             "Unable to parse '{}'.",
             stat_path.string());

  std::istringstream stat_tokens{stat_line.substr(comm_end + 2)};
  std::vector<std::string> fields;
  for (std::string field; stat_tokens >> field;) fields.push_back(field);

  constexpr size_t utime_index = 11;
  constexpr size_t stime_index = 12;
  TIT_ENSURE(fields.size() > stime_index,
             "Unexpected format in '{}'.",
             stat_path.string());

  const auto utime = std::stoull(fields[utime_index]);
  const auto stime = std::stoull(fields[stime_index]);

  const auto statm_path =
      std::filesystem::path{"/proc"} / std::to_string(pid) / "statm";
  std::ifstream statm_stream{statm_path};
  TIT_ENSURE(statm_stream.is_open(),
             "Unable to open '{}'.",
             statm_path.string());

  uint64_t size_pages = 0;
  uint64_t resident_pages = 0;
  statm_stream >> size_pages >> resident_pages;
  TIT_ENSURE(statm_stream.good(), "Unable to parse '{}'.", statm_path.string());

  return {
      .cpu_time_ns = to_ns(utime + stime, sys_info::clock_ticks_per_second()),
      .memory_bytes = resident_pages * sys_info::page_size(),
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::proc_info

#endif // ifdef __linux__
