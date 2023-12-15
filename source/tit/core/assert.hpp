/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

#include "tit/core/config.hpp"

#if TIT_GCOV
extern "C" void __gcov_dump(); // NOLINT
#endif

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** "Assume" macro: assume that given expression always holds. Use this macro
 ** to tell the compiler some not-obvious facts to better optimize the code. */
#define TIT_ASSUME(expr) [[assume(expr)]]

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** "Ensure" macro: ALWAYS (even in non-debug builds) check that given
 ** expression holds. Do not use this macro for user input, but just to check
 ** the internal logic. If the expression does not hold, the entire process
 ** is aborted (in `constexpr` context an exception is thrown), so it this macro
 ** is intended to be used inside of `noexcept` functions. */
#define TIT_ENSURE(expr, message) (tit::_ensure((expr), #expr, (message)))

/** "Assert" macro: ensure that given expression holds in debug builds,
 ** and assume it always holds in non-debug build (which leads to better
 ** optimizations). */
#ifdef NDEBUG
#define TIT_ASSERT(expr, message, ...) TIT_ASSUME((expr))
#else
#define TIT_ASSERT(expr, message, ...) TIT_ENSURE((expr), (message))
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Abort the current process in non-`constexpr` context.
[[noreturn]] inline void _ensure_failed( //
    std::string_view expr_string, std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  // TODO: print stack trace.
  std::cerr << location.file_name() << ":" << location.line() << ":"
            << location.column() << ": ";
  std::cerr << "Internal consistency check failed: \"" << expr_string << "\". ";
  std::cerr << message << "\n";
  std::cerr.flush();
#if TIT_GCOV
  // Dump coverage data.
  __gcov_dump();
#endif
  std::_Exit(-1);
}

// Actual implementation of "Ensure" macro.
constexpr void _ensure( //
    bool expr_result, std::string_view expr_string, std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  if (expr_result) return; // Gracefully return if check passes.
  _ensure_failed(expr_string, message, location); // Abort process otherwise.
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
