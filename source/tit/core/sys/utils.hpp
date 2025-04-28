/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <filesystem>
#include <optional>

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

/// Path to the current executable.
auto exe_path() -> std::filesystem::path;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Terminal stream type.
enum class TTY : uint8_t {
  Stdout = STDOUT_FILENO, ///< Standard output.
  Stderr = STDERR_FILENO, ///< Standard error.
};

/// Query terminal width.
auto tty_width(TTY tty) -> std::optional<size_t>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
