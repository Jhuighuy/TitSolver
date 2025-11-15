/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <source_location>
#include <string_view>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that the given expression holds (always).
#define TIT_ALWAYS_ASSERT(expr, message)                                       \
  tit::checks::run_assert((expr), (#expr), message)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Should we enable assertions? By default, we enable them in debug mode.
#if !defined(TIT_ENABLE_ASSERTS) && !defined(NDEBUG)
#define TIT_ENABLE_ASSERTS
#endif

/// Check that the given expression holds (if assertions are enabled).
///
/// If the expression does not hold, abort the entire process (or abort the
/// compilation, if the expression is evaluated in `constexpr` context).
///
/// Use this macro to check the internal logic, do not use it to check the
/// status of any operation (in such case, use `TIT_ALWAYS_ASSERT` or
/// `TIT_ENSURE` instead).
#ifdef TIT_ENABLE_ASSERTS
#define TIT_ASSERT(expr, message) TIT_ALWAYS_ASSERT(expr, message)
#else
#define TIT_ASSERT(expr, message) static_cast<void>(expr)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit::checks {

// Report an assertion failure.
// Note: Implemented in `main.cpp`.
[[noreturn]] void report_assert_failure(std::string_view expression,
                                        std::string_view message,
                                        std::source_location location) noexcept;

// Check that the given expression holds.
[[gnu::always_inline]] constexpr void run_assert(
    bool condition,
    std::string_view expression,
    std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  if (!condition) report_assert_failure(expression, message, location);
}

} // namespace tit::checks

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
