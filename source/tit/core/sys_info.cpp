/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <format>
#include <string>

#include <sys/utsname.h>
#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys_info.hpp"

namespace tit::sys_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

#define checked_sysconf(name)                                                  \
  [] {                                                                         \
    const auto result = sysconf(name);                                         \
    TIT_ENSURE(result != -1, "`sysconf({})` failed.", #name);                  \
    return result;                                                             \
  }()

void checked_uname(utsname& uts) {
  const auto status = uname(&uts);
  TIT_ENSURE(status == 0, "`uname()` failed.");
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto host_name() -> std::string {
  std::array<char, 256> buffer{};
  const auto status = gethostname(buffer.data(), buffer.size() - 1);
  TIT_ENSURE(status == 0, "Unable to query the host name.");
  return buffer.data();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto kernel_name_and_version() -> std::string {
  utsname uts{};
  checked_uname(uts);
  return std::format("{} {}", uts.sysname, uts.release);
}

auto os_info() -> std::string {
  return std::format("{} ({})",
                     dist_name_and_version(),
                     kernel_name_and_version());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto cpu_arch() -> std::string {
  utsname uts{};
  checked_uname(uts);
  return static_cast<const char*>(uts.machine);
}

auto cpu_cores() -> uint64_t {
  return checked_sysconf(_SC_NPROCESSORS_CONF);
}

auto cpu_info() -> std::string {
  const auto num_sockets = cpu_sockets();
  TIT_ENSURE(num_sockets > 0, "Number of CPU sockets must be positive.");
  auto result = std::format("{} ({} × {}), {}",
                            cpu_name(),
                            cpu_perf_cores() / num_sockets,
                            fmt_quantity(cpu_perf_core_frequency(), "Hz"),
                            cpu_arch());
  if (num_sockets > 1) result = std::format("{} × {}", num_sockets, result);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto ram_size() -> uint64_t {
  const auto pages = checked_sysconf(_SC_PHYS_PAGES);
  const auto page_size = checked_sysconf(_SC_PAGE_SIZE);
  return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sys_info
