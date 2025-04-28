/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <unistd.h>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// POSIX file descriptor.
enum class fd_t : int {};

/// Standard input file descriptor.
inline constexpr fd_t stdin_fd{STDIN_FILENO};

/// Standard output file descriptor.
inline constexpr fd_t stdout_fd{STDOUT_FILENO};

/// Standard error file descriptor.
inline constexpr fd_t stderr_fd{STDERR_FILENO};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if a file descriptor is a terminal.
auto is_terminal(fd_t fd) -> bool;

/// Default terminal width.
inline constexpr size_t default_terminal_width = 80;

/// Query width of a terminal.
/// If the file descriptor is not a terminal, returns default width (80).
auto terminal_width(fd_t fd) -> size_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
