/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <filesystem>
#include <string>

#include <unistd.h>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// At-exit callback function.
using atexit_callback_t = void (*)();

/// Register a function to be called at exit.
void checked_atexit(atexit_callback_t callback);

/// Exit code.
enum class ExitCode : uint8_t {
  success = 0, ///< Success.
  failure = 1, ///< Failure.
};

/// Exit from the current process.
[[noreturn]] void exit(ExitCode exit_code) noexcept;

/// Fast-exit from the current process.
///
/// @note No at-exit callbacks are triggered, except for coverage report.
[[noreturn]] void fast_exit(ExitCode exit_code) noexcept;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get host name.
auto host_name() -> std::string;

/// Get distribution name and version.
auto dist_name_and_version() -> std::string;

/// Get kernel name and version.
auto kernel_name_and_version() -> std::string;

/// Get overall OS information.
auto os_info() -> std::string;

/// Get CPU architecture.
auto cpu_arch() -> std::string;

/// Get CPU name.
auto cpu_name() -> std::string;

/// Get number of CPU sockets.
auto cpu_sockets() -> uint64_t;

/// Get number of all (logical) CPU cores.
auto cpu_cores() -> uint64_t;

/// Get number of performance (logical) CPU cores.
auto cpu_perf_cores() -> uint64_t;

/// Get CPU frequency in Hz.
auto cpu_perf_core_frequency() -> uint64_t;

/// Get CPU overall CPU information.
auto cpu_info() -> std::string;

/// Get RAM size in bytes.
auto ram_size() -> uint64_t;

/// Get available disk space in bytes.
auto disk_space() -> uint64_t;

/// Get path to the current executable.
auto exe_path() -> std::filesystem::path;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Terminal stream type.
enum class TTY : uint8_t {
  Stdout = STDOUT_FILENO, ///< Standard output.
  Stderr = STDERR_FILENO, ///< Standard error.
};

/// Query terminal width. If this information is not available
// (e.g. redirected), return default value of 80 characters.
auto tty_width(TTY tty) -> size_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
