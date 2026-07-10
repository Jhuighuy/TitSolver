/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "tit/sym/expression.hpp"

namespace tit::sym {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Estimated cost of evaluating a symbolic expression.
struct ExpressionCost final {
  std::size_t operations = 0;
  std::size_t nodes = 0;

  friend auto operator==(ExpressionCost, ExpressionCost) noexcept
      -> bool = default;
};

/// Estimate the run-time and structural cost of an expression.
auto expression_cost(const Expr& expression) -> ExpressionCost;

/// A named intermediate expression.
struct ExpressionBinding final {
  Expr symbol;
  Expr value;
};

/// Expression represented as an evaluation-ordered sequence of bindings.
class OptimizedExpr final {
public:

  OptimizedExpr(std::vector<ExpressionBinding> bindings, Expr result);

  /// Intermediate bindings in evaluation order.
  auto bindings() const noexcept -> const std::vector<ExpressionBinding>&;

  /// Final result expression.
  auto result() const noexcept -> const Expr&;

  /// Does the complete program use `symbol` as an input?
  auto uses(const Expr& symbol) const -> bool;

  /// Total cost of all binding values and the result.
  auto cost() const -> ExpressionCost;

private:

  std::vector<ExpressionBinding> bindings_;
  Expr result_;

}; // class OptimizedExpr

/// Eliminate profitable exact common subexpressions.
auto optimize(const Expr& expression, std::string temp_prefix = "x")
    -> OptimizedExpr;

/// Predicate selecting a non-root expression to materialize as a temporary.
using SpillPredicate = std::function<bool(const Expr&, ExpressionCost)>;

/// Materialize selected large subexpressions, including single-use ones.
auto spill(const OptimizedExpr& expression,
           const SpillPredicate& predicate,
           std::string temp_prefix = "x") -> OptimizedExpr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
