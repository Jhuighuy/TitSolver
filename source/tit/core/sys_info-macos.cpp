/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __APPLE__

#include <array>
#include <format>
#include <string>

#include <sys/sysctl.h>
#include <sys/time.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys_info.hpp"

namespace tit::sys_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto checked_sysctlbyname(const char* name) -> std::string {
  std::array<char, 256> result{};
  auto size = result.size() - 1;
  const auto status = sysctlbyname(name, result.data(), &size, nullptr, 0);
  TIT_ENSURE(status == 0, "sysctlbyname('{}') failed.", name);
  return result.data();
}

template<class T>
auto checked_sysctlbyname(const char* name) -> T {
  T result{};
  auto size = sizeof(result);
  const auto status = sysctlbyname(name, &result, &size, nullptr, 0);
  TIT_ENSURE(status == 0, "sysctlbyname('{}') failed.", name);
  return result;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto dist_name_and_version() -> std::string {
  // Note: For SDK versions < 11.0, the "kern.osproductversion" will return
  // the value of "kern.osproductversioncompat" (10.16), which is not useful.
  return std::format("macOS {}", checked_sysctlbyname("kern.osproductversion"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto cpu_name() -> std::string {
  return checked_sysctlbyname("machdep.cpu.brand_string");
}

auto cpu_sockets() -> uint64_t {
  return checked_sysctlbyname<uint64_t>("hw.packages");
}

auto cpu_perf_cores() -> uint64_t {
  try {
    return checked_sysctlbyname<uint64_t>("hw.perflevel0.logicalcpu_max");
  } catch (const Exception& /*e*/) {
    return checked_sysctlbyname<uint64_t>("hw.logicalcpu_max");
  }
}

auto cpu_perf_core_frequency() -> uint64_t {
  // Note: "hw.cpufrequency[_max]" may not be available on Apple Silicon. See:
  // https://github.com/giampaolo/psutil/issues/1892#issuecomment-1187911499
  try {
    return checked_sysctlbyname<uint64_t>("hw.cpufrequency_max");
  } catch (const Exception& /*e*/) {
    return checked_sysctlbyname<uint64_t>("hw.tbfrequency") *
           checked_sysctlbyname<clockinfo>("kern.clockrate").hz;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sys_info

#endif // ifdef __APPLE__
