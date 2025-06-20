/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdio> // IWYU pragma: keep
#include <format>
#include <print>
#include <utility>

namespace tit {

using std::print;
using std::println;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the formatted string to the standard error stream.
template<class... Args>
void eprint(std::format_string<Args...> fmt, Args&&... args) {
  print(stderr, fmt, std::forward<Args>(args)...);
}

/// Print the formatted string with a new line to the standard error stream.
template<class... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  println(stderr, fmt, std::forward<Args>(args)...);
}

/// Print the newline to the standard error stream.
inline void eprintln() {
  eprintln("");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print information message.
template<class... Args>
void log(std::format_string<Args...> fmt, Args&&... args) {
  print("INFO: "), println(fmt, std::forward<Args>(args)...);
}

/// Print warning message.
template<class... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
  eprint("WARN: "), eprintln(fmt, std::forward<Args>(args)...);
}

/// Print error message.
template<class... Args>
void err(std::format_string<Args...> fmt, Args&&... args) {
  eprint("ERROR: "), eprintln(fmt, std::forward<Args>(args)...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
