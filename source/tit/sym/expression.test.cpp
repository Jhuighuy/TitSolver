/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <compare>

#include "tit/sym/expression.hpp"
#include "tit/sym/rational.hpp"
#include "tit/testing/test.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sym::Expr literals") {
  const Context context;
  const auto zero = context.integer(0);
  const auto half = context.rational(1, 2);
  const auto x = context.symbol("x");

  CHECK(zero.is_zero());
  CHECK(zero.is_integer());
  CHECK(zero.as_integer() == 0);
  CHECK(half.as_rational() == Rational{1, 2});
  CHECK(x.is_symbol());
  CHECK(x.symbol_name() == "x");
  CHECK(context.symbol("x") == x);
}

TEST_CASE("sym::Expr canonical addition") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");

  CHECK(x + 0 == x);
  CHECK(0 + x == x);
  CHECK(x + y == y + x);
  CHECK(x + x == 2 * x);
  CHECK(2 * x + 3 * x == 5 * x);
  CHECK(x - x == context.integer(0));
  CHECK((x + y) + 1 == 1 + (y + x));
}

TEST_CASE("sym::Expr canonical multiplication") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");

  CHECK(x * 0 == context.integer(0));
  CHECK(x * 1 == x);
  CHECK(x * y == y * x);
  CHECK(x * x == pow(x, 2));
  CHECK(pow(x, 2) * pow(x, 3) == pow(x, 5));
  CHECK((2 * x) * (3 * y) == 6 * x * y);
  CHECK(x / x == context.integer(1));
}

TEST_CASE("sym::Expr powers") {
  const Context context;
  const auto x = context.symbol("x");
  const auto n = context.symbol("n");

  CHECK(pow(x, 0) == context.integer(1));
  CHECK(pow(x, 1) == x);
  CHECK(pow(pow(x, 2), 3) == pow(x, 6));
  CHECK(pow(context.rational(2, 3), -2) == context.rational(9, 4));
  CHECK(pow(x, n).kind() == ExprKind::pow);
}

TEST_CASE("sym::Expr deterministic structure") {
  const Context first;
  const Context second;
  const auto a = first.symbol("a") + 2 * first.symbol("b");
  const auto b = 2 * second.symbol("b") + second.symbol("a");

  CHECK(a == b);
  CHECK(a.hash() == b.hash());
  CHECK((a <=> b) == std::strong_ordering::equal);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sym
