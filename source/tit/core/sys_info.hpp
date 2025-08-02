/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#pragma once

#include <string>

#include "tit/core/basic_types.hpp"

namespace tit::sys_info {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get host name.
auto host_name() -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get distribution name and version.
auto dist_name_and_version() -> std::string;

/// Get kernel name and version.
auto kernel_name_and_version() -> std::string;

/// Get overall OS information.
auto os_info() -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

/// Get overall CPU information.
auto cpu_info() -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get RAM size in bytes.
auto ram_size() -> uint64_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sys_info
