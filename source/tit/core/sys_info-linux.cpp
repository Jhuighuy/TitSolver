/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __linux__

#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/str_hash_set.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys_info.hpp"

namespace tit::sys_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto dist_name_and_version() -> std::string {
  for (const auto* const path : {"/etc/os-release", "/usr/lib/os-release"}) {
    std::ifstream file{path};
    if (!file.is_open()) continue;

    std::optional<std::string> name;
    std::optional<std::string> version_id;
    for (std::string line; std::getline(file, line);) {
      if (constexpr std::string_view name_prefix = "NAME=";
          line.starts_with(name_prefix)) {
        name = line.substr(name_prefix.size());
      }
      if (constexpr std::string_view version_id_prefix = "VERSION_ID=";
          line.starts_with(version_id_prefix)) {
        version_id = line.substr(version_id_prefix.size());
      }
    }

    TIT_ENSURE(name.has_value(), "Cannot find any 'NAME' in '{}'.", path);
    if (version_id.has_value()) return std::format("{} {}", *name, *version_id);
    return *name;
  }

  TIT_THROW("Cannot locate 'os-release' file.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto query_cpuinfo(std::string_view key) -> std::vector<std::string> {
  std::ifstream stream{"/proc/cpuinfo"};
  TIT_ENSURE(stream.is_open(), "Unable to open '/proc/cpuinfo'.");

  std::vector<std::string> result{};
  for (std::string line; std::getline(stream, line);) {
    if (!line.contains(key)) continue;
    result.push_back(line.substr(line.find(':') + /*len(": ")=*/2));
  }

  TIT_ENSURE(!result.empty(), "Cannot find any '{}' in '/proc/cpuinfo'.", key);
  return result;
}

auto query_cpufreq() -> std::vector<uint64_t> {
  std::vector<uint64_t> result{};
  for (size_t core = 0; core < cpu_cores(); ++core) {
    std::ifstream file{
        std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq",
                    core)};
    if (!file.is_open()) continue;

    uint64_t result_kHz{};
    file >> result_kHz;
    result.push_back(result_kHz * 1000);
  }

  TIT_ENSURE(!result.empty(), "Cannot find any 'cpufreq' in '/sys/devices'.");
  return result;
}

} // namespace

auto cpu_name() -> std::string {
  return query_cpuinfo("model name").front();
}

auto cpu_sockets() -> uint64_t {
  return (query_cpuinfo("physical id") | std::ranges::to<StrHashSet>()).size();
}

auto cpu_perf_cores() -> uint64_t {
  std::map<uint64_t, uint64_t, std::greater<>> core_counts{};
  for (const auto freq : query_cpufreq()) ++core_counts[freq];
  return core_counts.begin()->second;
}

auto cpu_perf_core_frequency() -> uint64_t {
  return query_cpufreq().front();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sys_info

#endif // ifdef __linux__
