/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <initializer_list>
#include <ranges>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#warnings"
#endif
#include <doctest/doctest.h> // IWYU pragma: exports
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace tit::testing {

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

/// Require the operands to be approximately equal.
#define REQUIRE_APPROX_EQ(...) REQUIRE(approx_equal_to(__VA_ARGS__))

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

/// Check that two ranges are equal.
template<std::ranges::input_range Range1,
         std::ranges::input_range Range2 =
             std::initializer_list<std::ranges::range_value_t<Range1>>>
constexpr auto equal(Range1&& a, Range2&& b) -> bool {
  return std::ranges::equal(a, b);
}

/// Test that the range operands are equal.
#define CHECK_RANGE_EQ(...) CHECK(tit::testing::equal(__VA_ARGS__))

/// Require the range operands to be equal.
#define REQUIRE_RANGE_EQ(...) REQUIRE(tit::testing::equal(__VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check that two ranges are approximately equal.
template<std::ranges::input_range Range1,
         std::ranges::input_range Range2 =
             std::initializer_list<std::ranges::range_value_t<Range1>>>
constexpr auto approx_equal(Range1&& a, Range2&& b) -> bool {
  return std::ranges::equal(a, b, [](const auto& ax, const auto& bx) {
    return approx_equal_to(ax, bx);
  });
}

/// Test that two ranges are approximately equal.
#define CHECK_RANGE_APPROX_EQ(...)                                             \
  CHECK(tit::testing::approx_equal(__VA_ARGS__))

/// Require that two ranges are approximately equal.
#define REQUIRE_RANGE_APPROX_EQ(...)                                           \
  REQUIRE(tit::testing::approx_equal(__VA_ARGS__))

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

} // namespace tit::testing
