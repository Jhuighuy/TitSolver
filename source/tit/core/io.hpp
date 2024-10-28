/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <iostream>
#include <print>
#include <utility>

#include "tit/core/missing.hpp"

namespace tit {

using std::print;
using std::println;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the formatted string to the standard output stream.
template<class... Args>
void eprint(std::format_string<Args...> fmt, Args&&... args) {
  print(std::cerr, fmt, std::forward<Args>(args)...);
}

/// Print the formatted string with a new line to the standard output stream.
template<class... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  println(std::cerr, fmt, std::forward<Args>(args)...);
}

/// Print the newline to the standard error stream.
inline void eprintln() {
  std::cerr << '\n';
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the stack trace to the standard error stream.
void eprint_stacktrace(
    const Std::stacktrace& stacktrace = Std::stacktrace::current());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
