/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdio>
#include <format>
#include <print>
#include <source_location>
#include <stacktrace>
#include <string_view>
#include <utility>

namespace tit {

using std::print;
using std::println;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the formatted string to the standard output stream.
template<class... Args>
void eprint(std::format_string<Args...> fmt, Args&&... args) {
  print(stderr, fmt, std::forward<Args>(args)...);
}

/// Print the formatted string with a new line to the standard output stream.
template<class... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  println(stderr, fmt, std::forward<Args>(args)...);
}

/// Print the newline to the standard error stream.
inline void eprintln() {
  eprintln("");
}

/// Print information message.
template<class... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
  print("INFO: "), println(fmt, std::forward<Args>(args)...);
}

/// Print warning message.
template<class... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
  eprint("WARN: "), eprintln(fmt, std::forward<Args>(args)...);
}

/// Print error message.
template<class... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
  eprint("ERROR: "), eprintln(fmt, std::forward<Args>(args)...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print a separator line.
/// @{
void println_separator(char c = '-');
void eprintln_separator(char c = '-');
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the logo and system information.
void println_logo_and_system_info();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print crash report.
void eprintln_crash_report(
    std::string_view message,
    std::string_view cause = "",
    std::string_view cause_description = "",
    std::source_location loc = std::source_location::current(),
    std::stacktrace trace = std::stacktrace::current());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
