/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <source_location>
#include <string_view>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fail with the given message.
///
/// Use this macro to abort the entire process due a failure in the internal
/// logic with in cases, when the actual check is done elsewhere, (e.g.,
/// when translating a library error code to a user-friendly message).
#define TIT_FAIL(message) tit::impl::handle_check_failure("TIT_FAIL()", message)

/// Check that the given expression holds.
///
/// Do not use this macro for user input, but just to check the internal logic.
/// If the expression does not hold, the entire process is aborted (in constant
/// evaluation context, compilation is aborted).
#define TIT_ENSURE(expr, message) tit::impl::run_check((expr), (#expr), message)

/// Check that the given expression holds (in debug mode).
///
/// Do not use this macro for user input, but just to check the internal logic.
/// If the expression does not hold, the entire process is aborted (in constant
/// evaluation context, compilation is aborted).
#ifdef NDEBUG
#define TIT_ASSERT(expr, message) static_cast<void>(expr)
#else
#define TIT_ASSERT(expr, message) TIT_ENSURE((expr), (message))
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit::impl {

[[noreturn]]
void handle_check_failure(
    std::string_view expression,
    std::string_view message,
    std::source_location location = std::source_location::current()) noexcept;

constexpr void run_check(
    bool condition,
    std::string_view expression,
    std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  if (condition) return; // Gracefully return if check passes.
  handle_check_failure(expression, message, location);
}

} // namespace tit::impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
