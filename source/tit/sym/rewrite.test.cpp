/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <optional>

#include "tit/sym/expression.hpp"
#include "tit/sym/rewrite.hpp"
#include "tit/testing/test.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sym::contains") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto expression = pow(x + 1, 2) + y;

  CHECK(contains(expression, x));
  CHECK(contains(expression, x + 1));
  CHECK(contains(expression, expression));
  CHECK_FALSE(contains(expression, context.symbol("z")));
}

TEST_CASE("sym::substitute") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");

  CHECK(substitute(pow(x, 2) + x, x, y + 1) == pow(y + 1, 2) + y + 1);
}

TEST_CASE("sym::rewrite_bottom_up") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto result = rewrite_bottom_up(
      pow(x, 4) + pow(y, 3),
      [&x](const Expr& expression) -> RewriteResult {
        if (expression.kind() == ExprKind::pow && expression[0] == x) {
          return pow(x, expression[1].as_integer() / 2);
        }
        return std::nullopt;
      });

  CHECK(result == pow(x, 2) + pow(y, 3));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sym
