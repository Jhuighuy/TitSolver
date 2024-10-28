/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <optional>
#include <string>
#include <typeinfo>

#include <unistd.h>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// At-exit callback function.
using atexit_callback_t = void (*)();

/// Register a function to be called at exit.
void checked_atexit(atexit_callback_t callback) noexcept;

/// Exit code.
enum class ExitCode : uint8_t {
  success = 0, ///< Success.
  failure = 1, ///< Failure.
};

/// Exit from the current process.
[[noreturn]]
void exit(ExitCode exit_code) noexcept;

/// Fast-exit from the current process.
///
/// @note No at-exit callbacks are triggered, except for coverage report.
[[noreturn]]
void fast_exit(ExitCode exit_code) noexcept;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Execute a system command. Fail if the command fails.
void checked_system(const char* command) noexcept;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Terminal stream type.
enum class TTY : uint8_t {
  Stdout = STDOUT_FILENO, ///< Standard output.
  Stderr = STDERR_FILENO, ///< Standard error.
};

/// Query terminal width.
auto tty_width(TTY tty) noexcept -> std::optional<size_t>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Try to demangle a mangled name.
/// @{
auto try_demangle(const char* mangled_name) -> std::optional<std::string>;
auto try_demangle_arg_type(const auto& arg) -> std::optional<std::string> {
  return try_demangle(typeid(arg).name());
}
template<class Type>
auto try_demangle_type() -> std::optional<std::string> {
  return try_demangle(typeid(Type).name());
}
/// @}

/// Try to demangle a mangled name.
/// If demangling fails, return the original name.
/// @{
auto maybe_demangle(const char* mangled_name) -> std::string;
auto maybe_demangle_arg_type(const auto& arg) -> std::string {
  return maybe_demangle(typeid(arg).name());
}
template<class Type>
auto maybe_demangle_type() -> std::string {
  return maybe_demangle(typeid(Type).name());
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
