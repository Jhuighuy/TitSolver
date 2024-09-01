/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// Doctest does not include `std::ostream`, but it does use it. We need to
// include it here to avoid compiler errors.
#include <ostream> // IWYU pragma: keep

#include <doctest/doctest.h> // IWYU pragma: exports

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Test that the expression holds, both in compile-time and run-time.
#define STATIC_CHECK(...)                                                      \
  do {                                                                         \
    static_assert(__VA_ARGS__);                                                \
    CHECK(__VA_ARGS__);                                                        \
  } while (false)

/// Test that the expression does not hold, both in compile-time and run-time.
#define STATIC_CHECK_FALSE(...)                                                \
  do {                                                                         \
    static_assert(!(__VA_ARGS__));                                             \
    CHECK_FALSE(__VA_ARGS__);                                                  \
  } while (false)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Test that the operands are approximately equal.
#define CHECK_APPROX_EQ(...) CHECK(approx_equal_to(__VA_ARGS__))

/// Test that the operands are not approximately equal.
#define CHECK_APPROX_NE(...) CHECK(!approx_equal_to(__VA_ARGS__))

/// Test that the operands are approximately equal.
#define REQUIRE_APPROX_EQ(...) REQUIRE(approx_equal_to(__VA_ARGS__))

/// Test that the operands are not approximately equal.
#define REQUIRE_APPROX_NE(...) REQUIRE(!approx_equal_to(__VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
