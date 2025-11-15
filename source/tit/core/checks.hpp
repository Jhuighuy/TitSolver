/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <source_location>
#include <string_view>

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
/// Use this macro to check the internal logic, do not use it for to check the
/// status of any operation (in such case, use `TIT_ENSURE` instead).
#ifdef TIT_ENABLE_ASSERTS
#define TIT_ASSERT(expr, message) tit::impl::do_assert((expr), (#expr), message)
#else
#define TIT_ASSERT(expr, message) static_cast<void>(expr)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace tit::impl {

// Note: Implemented in `main.cpp`.
[[noreturn]] void report_assert_failure(std::string_view expression,
                                        std::string_view message,
                                        std::source_location location) noexcept;

[[gnu::always_inline]] constexpr void do_assert(
    bool condition,
    std::string_view expression,
    std::string_view message,
    std::source_location location = std::source_location::current()) noexcept {
  if (!condition) report_assert_failure(expression, message, location);
}

} // namespace tit::impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
