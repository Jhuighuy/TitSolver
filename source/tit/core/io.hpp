/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <iostream>
#include <utility>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the formatted string to the standard output stream.
template<class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
  std::cout << std::format(fmt, std::forward<Args>(args)...);
}
/// Print the formatted string with a new line to the standard output stream.
template<class... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
  std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

/// Print the formatted string to the standard output stream.
template<class... Args>
void eprint(std::format_string<Args...> fmt, Args&&... args) {
  std::cerr << std::format(fmt, std::forward<Args>(args)...);
}
/// Print the formatted string with a new line to the standard output stream.
template<class... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  std::cerr << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
