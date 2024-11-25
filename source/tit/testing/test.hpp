/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// Doctest does not include `std::ostream`, but it does use it. We need to
// include it here to avoid compiler errors.
#include <ostream> // IWYU pragma: keep

// Keep the algorithm header for range comparisons.
#include <algorithm> // IWYU pragma: keep

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

/// Require the operands to be approximately equal.
#define REQUIRE_APPROX_EQ(...) REQUIRE(approx_equal_to(__VA_ARGS__))

/// Require the operands not to be approximately equal.
#define REQUIRE_APPROX_NE(...) REQUIRE(!approx_equal_to(__VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Test that the range operand is empty.
#define CHECK_RANGE_EMPTY(...) CHECK(std::ranges::empty(__VA_ARGS__))

/// Test that the range operand is not empty.
#define CHECK_RANGE_NOT_EMPTY(...) CHECK(!std::ranges::empty(__VA_ARGS__))

/// Require the range operand to be empty.
#define REQUIRE_RANGE_EMPTY(...) REQUIRE(std::ranges::empty(__VA_ARGS__))

/// Require the range operand not to be empty.
#define REQUIRE_RANGE_NOT_EMPTY(...) REQUIRE(!std::ranges::empty(__VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Test that the range operands are equal.
#define CHECK_RANGE_EQ(...) CHECK(std::ranges::equal(__VA_ARGS__))

/// Test that the range operands are not equal.
#define CHECK_RANGE_NE(...) CHECK_FALSE(std::ranges::equal(__VA_ARGS__))

/// Require the range operands to be equal.
#define REQUIRE_RANGE_EQ(...) REQUIRE(std::ranges::equal(__VA_ARGS__))

/// Require the range operands not to be equal.
#define REQUIRE_RANGE_NE(...) REQUIRE_FALSE(std::ranges::equal(__VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Test that the expression throws an exception, whose message contains the
/// expected substring.
#define CHECK_THROWS_MSG(expr, Exception, substring)                           \
  CHECK_THROWS_WITH_AS(expr, doctest::Contains(substring), Exception)

/// Require the expression throws an exception, whose message contains the
/// expected substring.
#define REQUIRE_THROWS_MSG(expr, Exception, substring)                         \
  REQUIRE_THROWS_WITH_AS(expr, doctest::Contains(substring), Exception)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
