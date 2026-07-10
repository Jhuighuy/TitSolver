/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <string_view>
#include <unordered_map>

#include "tit/sym/expression.hpp"
#include "tit/sym/optimize.hpp"
#include "tit/sym/rational.hpp"
#include "tit/testing/test.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto evaluate(const Expr& expression,
              const std::unordered_map<std::string_view, Rational>& symbols)
    -> Rational { // NOLINT(*-no-recursion)
  if (expression.is_rational()) return expression.as_rational();
  if (expression.is_symbol()) return symbols.at(expression.symbol_name());
  if (expression.kind() == ExprKind::add) {
    Rational result{0};
    for (std::size_t i = 0; i < expression.arity(); ++i) {
      result += evaluate(expression[i], symbols);
    }
    return result;
  }
  if (expression.kind() == ExprKind::mul) {
    Rational result{1};
    for (std::size_t i = 0; i < expression.arity(); ++i) {
      result *= evaluate(expression[i], symbols);
    }
    return result;
  }
  auto exponent = expression[1].as_integer();
  auto base = evaluate(expression[0], symbols);
  Rational result{1};
  const bool inverse = exponent < 0;
  if (inverse) exponent = -exponent;
  while (exponent-- > 0) result *= base;
  return inverse ? Rational{1} / result : result;
}

auto evaluate(const OptimizedExpr& expression,
              std::unordered_map<std::string_view, Rational> symbols)
    -> Rational {
  for (const auto& binding : expression.bindings()) {
    symbols.insert_or_assign(binding.symbol.symbol_name(),
                             evaluate(binding.value, symbols));
  }
  return evaluate(expression.result(), symbols);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sym::expression_cost") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");

  CHECK(expression_cost(x) == ExpressionCost{.operations = 0, .nodes = 1});
  CHECK(expression_cost(x + y) == ExpressionCost{.operations = 1, .nodes = 3});
  CHECK(expression_cost(pow(x + y, 3)).operations == 3);
}

TEST_CASE("sym::optimize") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto common = x + y;
  const auto original = pow(common, 2) + pow(common, 3);
  const auto optimized = optimize(original);

  REQUIRE(optimized.bindings().size() == 1);
  CHECK(optimized.bindings().front().value == common);
  CHECK(optimized.bindings().front().symbol.symbol_name() == "x0");
  CHECK(optimized.uses(x));
  CHECK(optimized.uses(y));
  CHECK(evaluate(optimized, {{"x", 2}, {"y", 3}}) ==
        evaluate(original, {{"x", 2}, {"y", 3}}));
  CHECK(optimized.cost().operations < expression_cost(original).operations);
}

TEST_CASE("sym::optimize avoids symbol collisions") {
  const Context context;
  const auto x0 = context.symbol("x0");
  const auto y = context.symbol("y");
  const auto common = x0 + y;
  const auto optimized = optimize(pow(common, 2) + pow(common, 3));

  REQUIRE(optimized.bindings().size() == 1);
  CHECK(optimized.bindings().front().symbol.symbol_name() == "x1");
}

TEST_CASE("sym::optimize is deterministic") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto common = x + y;
  const auto expression =
      pow(common, 2) + pow(common, 3) + pow(x - y, 2) + pow(x - y, 3);
  const auto first = optimize(expression);
  const auto second = optimize(expression);

  REQUIRE(first.bindings().size() == second.bindings().size());
  for (std::size_t i = 0; i < first.bindings().size(); ++i) {
    CHECK(first.bindings()[i].symbol == second.bindings()[i].symbol);
    CHECK(first.bindings()[i].value == second.bindings()[i].value);
  }
  CHECK(first.result() == second.result());
}

TEST_CASE("sym::spill") {
  const Context context;
  const auto x = context.symbol("x");
  const auto y = context.symbol("y");
  const auto expression = OptimizedExpr{{}, pow(1 + x * (2 + y), 2)};
  const auto spilled =
      spill(expression, [](const Expr& /*expression*/, ExpressionCost cost) {
        return cost.operations >= 2;
      });

  CHECK_RANGE_NOT_EMPTY(spilled.bindings());
  CHECK(evaluate(spilled, {{"x", 2}, {"y", 3}}) ==
        evaluate(expression, {{"x", 2}, {"y", 3}}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sym
