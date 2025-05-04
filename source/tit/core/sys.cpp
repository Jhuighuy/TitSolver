/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __linux__
#include <fstream>
#endif
#include <array>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <utility>
#ifdef __linux__
#include <fstream>
#include <vector>
#endif

#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/ttycom.h>
#endif

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys.hpp"
#ifdef __linux__
#include "tit/core/math.hpp"
#endif

#ifdef TIT_HAVE_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_atexit(atexit_callback_t callback) {
  TIT_ASSERT(callback != nullptr, "At-exit callback is invalid!");
  const auto status = std::atexit(callback);
  TIT_ENSURE(status == 0, "Unable to register at-exit callback!");
}

[[noreturn]] void exit(ExitCode exit_code) noexcept {
  std::exit(std::to_underlying(exit_code)); // NOLINT(concurrency-mt-unsafe)
}

[[noreturn]] void fast_exit(ExitCode exit_code) noexcept {
#ifdef TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(std::to_underlying(exit_code));
}

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

#ifdef __APPLE__
template<class T>
auto try_sysctlbyname(const char* name, T& result) -> bool {
  size_t size = sizeof(result);
  return sysctlbyname(name, &result, &size, nullptr, 0) == 0;
}
template<class T>
auto checked_sysctlbyname(const char* name) -> T {
  T result{};
  const auto status = try_sysctlbyname(name, result);
  TIT_ENSURE(status, "sysctlbyname('{}') failed.", name);
  return result;
}
auto checked_sysctlbyname(const char* name) -> std::string {
  return checked_sysctlbyname<std::array<char, 256>>(name).data();
}
#endif

#ifdef __linux__
auto query_cpuinfo(std::string_view key, size_t max_size = npos)
    -> std::vector<std::string> {
  std::ifstream cpuinfo("/proc/cpuinfo");
  TIT_ENSURE(cpuinfo.is_open(), "Unable to open '/proc/cpuinfo'.");
  std::vector<std::string> result{};
  for (std::string line; std::getline(cpuinfo, line);) {
    if (line.find(key) != std::string::npos) {
      result.push_back(line.substr(line.find(':') + /*len(": ")=*/2));
      if (result.size() >= max_size) break;
    }
  }
  TIT_ENSURE(!result.empty(), "cannot find any '{}' in '/proc/cpuinfo'.", key);
  return result;
}
#endif

} // namespace

auto host_name() -> std::string {
  std::array<char, 256> buffer{};
  const auto status = gethostname(buffer.data(), buffer.size() - 1);
  TIT_ENSURE(status == 0, "Unable to query the host name.");
  return buffer.data();
}

auto dist_name_and_version() -> std::string {
#ifdef __APPLE__
  // Note: For SDK versions < 11.0, the "kern.osproductversion" will return
  // the value of "kern.osproductversioncompat" (10.16), which is not useful.
  return std::format("macOS {}", checked_sysctlbyname("kern.osproductversion"));
#elifdef __linux__
  return "Linux"; /// @todo Implement properly for Linux.
#endif
}

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

auto cpu_arch() -> std::string {
  utsname uts{};
  checked_uname(uts);
  return static_cast<const char*>(uts.machine);
}

auto cpu_name() -> std::string {
#ifdef __APPLE__
  return checked_sysctlbyname("machdep.cpu.brand_string");
#elifdef __linux__
  return query_cpuinfo("model name", /*max_size=*/1).front();
#endif
}

auto cpu_sockets() -> uint64_t {
#ifdef __APPLE__
  return checked_sysctlbyname<uint64_t>("hw.packages");
#elifdef __linux__
  return (query_cpuinfo("physical id") | std::ranges::to<StrHashSet>()).size();
#endif
}

auto cpu_cores() -> uint64_t {
  return checked_sysconf(_SC_NPROCESSORS_CONF);
}

auto cpu_perf_cores() -> uint64_t {
#ifdef __APPLE__
  return checked_sysctlbyname<uint64_t>("hw.perflevel0.logicalcpu_max");
#elifdef __linux__
  return cpu_cores(); /// @todo Implement properly for Linux.
#endif
}

auto cpu_perf_core_frequency() -> uint64_t {
#ifdef __APPLE__
  // Note: "hw.cpufrequency[_max]" may not be available on Apple Silicon. See:
  // https://github.com/giampaolo/psutil/issues/1892#issuecomment-1187911499
  if (uint64_t f = 0; try_sysctlbyname("hw.cpufrequency_max", f)) return f;
  return checked_sysctlbyname<uint64_t>("hw.tbfrequency") *
         checked_sysctlbyname<clockinfo>("kern.clockrate").hz;
#elifdef __linux__
  return str_to<float64_t>(query_cpuinfo("cpu MHz", /*max_size=*/1).front())
      .transform([](float64_t freq) {
        return static_cast<uint64_t>(round(freq * 1e+6));
      })
      .value_or(0);
#endif
}

auto cpu_info() -> std::string {
  const auto num_sockets = cpu_sockets();
  auto result = std::format("{} ({} × {}), {}",
                            cpu_name(),
                            cpu_perf_cores() / num_sockets,
                            fmt_measurement(cpu_perf_core_frequency(), "Hz"),
                            cpu_arch());
  if (num_sockets > 1) result = std::format("{} × {}", num_sockets, result);
  return result;
}

auto ram_size() -> uint64_t {
  const auto pages = checked_sysconf(_SC_PHYS_PAGES);
  const auto page_size = checked_sysconf(_SC_PAGE_SIZE);
  return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
}

auto disk_space() -> uint64_t {
  struct statvfs stat{};
  const auto status = statvfs(std::filesystem::current_path().c_str(), &stat);
  TIT_ENSURE(status == 0, "Unable to query disk space.");
  return static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
}

auto exe_path() -> std::filesystem::path {
#ifdef __APPLE__
  std::array<char, PROC_PIDPATHINFO_MAXSIZE + 1> buffer{};
  const auto status = proc_pidpath(getpid(), buffer.data(), buffer.size() - 1);
  TIT_ENSURE(status > 0, "Unable to query the current executable path.");
  return buffer.data();
#elifdef __linux__
  return std::filesystem::canonical("/proc/self/exe");
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tty_width(TTY tty) -> size_t {
  const auto tty_fileno = std::to_underlying(tty);
  if (isatty(tty_fileno) == 0) return 80; // Redirected, return default value.

  winsize window_size = {}; // NOLINTNEXTLINE(*-vararg,*-include-cleaner)
  const auto status = ioctl(tty_fileno, TIOCGWINSZ, &window_size);
  TIT_ENSURE(status == 0,
             "Unable to query terminal window size with fileno {}!",
             tty_fileno);

  return window_size.ws_col;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
