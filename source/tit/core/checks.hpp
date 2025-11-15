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

/// Check that the given expression holds (if assertions are enabled).
///
/// If the expression does not hold, abort the entire process (or abort the
/// compilation, if the expression is evaluated in `constexpr` context).
///
/// Use this macro to check the internal logic, do not use it to check the
/// status of any operation (in such case, use `TIT_ALWAYS_ASSERT` or
/// `TIT_ENSURE` instead).
#ifdef NDEBUG
#define TIT_ASSERT(expr, message) static_cast<void>(expr)
#else
#define TIT_ASSERT(expr, message) TIT_ALWAYS_ASSERT(expr, message)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit::checks {

// Report an assertion failure.
//
// Note: Implemented in `main.cpp`.
[[noreturn]] void report_assert_failure(std::string_view expression,
                                        std::string_view message,
                                        std::source_location location) noexcept;

// Check that the given expression holds.
//
// Note: In debug (and coverage) mode, this function should never be inlined,
//       otherwise all assertions will be reported as partially covered. We do
//       not test assertions, so it makes sense to mark all assertions as
//       fully covered, if they are executed during the test.
#ifdef NDEBUG
[[gnu::always_inline]]
#else
[[gnu::noinline]]
#endif
constexpr void run_assert(
    bool condition,
    std::string_view expression,
    std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  if (!condition) report_assert_failure(expression, message, location);
}

} // namespace tit::checks

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
