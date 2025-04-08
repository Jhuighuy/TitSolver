/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>

#include <unistd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/str.hpp"

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

/// Get the value of an environment variable.
/// @{
auto get_env(CStrView name) noexcept -> std::optional<std::string_view>;
template<class Val>
auto get_env(CStrView name) noexcept -> std::optional<Val> {
  return get_env(name).and_then(str_to<Val>);
}
template<class Val>
auto get_env(CStrView name, Val fallback) noexcept -> Val {
  return get_env<Val>(name).value_or(fallback);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Try to demangle a mangled name.
/// @{
auto try_demangle(CStrView mangled_name) -> std::optional<std::string>;
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
auto maybe_demangle(CStrView mangled_name) -> std::string;
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
