/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <format> // IWYU pragma: keep
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

/// Subcase, name is runtime-formatted.
#define FSUBCASE(...) SUBCASE(std::format(__VA_ARGS__).c_str())

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

/// Check that two ranges are equal.
template<std::ranges::input_range Range1,
         std::ranges::input_range Range2 =
             std::initializer_list<std::ranges::range_value_t<Range1>>>
constexpr auto equal(Range1&& a, Range2&& b) -> bool {
  return std::ranges::equal(std::forward<Range1>(a), std::forward<Range2>(b));
}

/// Test that the range operands are equal.
#define CHECK_RANGE_EQ(...) CHECK(tit::testing::equal(__VA_ARGS__))

/// Test that the range operands are not equal.
#define CHECK_RANGE_NE(...) CHECK_FALSE(tit::testing::equal(__VA_ARGS__))

/// Require the range operands to be equal.
#define REQUIRE_RANGE_EQ(...) REQUIRE(tit::testing::equal(__VA_ARGS__))

/// Require the range operands not to be equal.
#define REQUIRE_RANGE_NE(...) REQUIRE_FALSE(tit::testing::equal(__VA_ARGS__))

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
