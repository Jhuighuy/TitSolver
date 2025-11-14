/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __linux__

#include <format>
#include <fstream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys_info.hpp"

namespace tit::sys_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto dist_name_and_version() -> std::string {
  constexpr auto eval_string = [](std::string_view str) {
    if (str.empty() || str.front() != '"') return std::string{str};
    try {
      const auto json = nlohmann::json::parse(str);
      TIT_ENSURE(json.is_string(), "JSON value '{}' is not a string.", str);
      return json.get<std::string>();
    } catch (const nlohmann::json::parse_error& e) {
      TIT_THROW("Failed to evaluate string '{}': {}", str, e.what());
    }
  };

  for (const auto* const path : {"/etc/os-release", "/usr/lib/os-release"}) {
    std::ifstream stream{path};
    if (!stream.is_open()) continue;

    std::optional<std::string> name;
    std::optional<std::string> version_id;
    for (std::string line; std::getline(stream, line);) {
      if (line.empty() || line.starts_with('#')) continue;

      if (constexpr std::string_view name_prefix = "NAME=";
          line.starts_with(name_prefix)) {
        name = eval_string(std::string_view{line}.substr(name_prefix.size()));
      }
      if (constexpr std::string_view version_id_prefix = "VERSION_ID=";
          line.starts_with(version_id_prefix)) {
        version_id = eval_string(
            std::string_view{line}.substr(version_id_prefix.size()));
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
  constexpr const auto* path = "/proc/cpuinfo";
  std::ifstream stream{path};
  TIT_ENSURE(stream.is_open(), "Unable to open '{}'.", path);

  std::vector<std::string> result{};
  for (std::string line; std::getline(stream, line);) {
    if (!line.contains(key)) continue;
    result.push_back(line.substr(line.find(':') + /*len(": ")=*/2));
  }

  TIT_ENSURE(!result.empty(), "Cannot find any '{}' in '{}'.", key, path);
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
  // Note: Linux currently has no way to robustly distinguish between
  //       performance and efficiency cores.
  return cpu_cores();
}

auto cpu_perf_core_frequency() -> uint64_t {
  constexpr const auto* path =
      "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
  std::ifstream stream{path};
  TIT_ENSURE(stream.is_open(), "Unable to open '{}'.", path);

  uint64_t result_kHz{};
  stream >> result_kHz;
  TIT_ENSURE(stream.good() && result_kHz > 0,
             "Failed to read valid CPU frequency from '{}'.",
             path);

  return result_kHz * 1000;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sys_info

#endif // ifdef __linux__
