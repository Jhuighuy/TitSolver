/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <optional>

#include "tit/sym/expression.hpp"

namespace tit::sym {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Result of a symbolic rewrite rule. An empty result leaves the node intact.
using RewriteResult = std::optional<Expr>;

/// Symbolic rewrite rule.
using RewriteRule = std::function<RewriteResult(const Expr&)>;

/// Does `expression` contain `subexpression`?
auto contains(const Expr& expression, const Expr& subexpression) -> bool;

/// Substitute `replacement` for every occurrence of `subexpression`.
auto substitute(const Expr& expression,
                const Expr& subexpression,
                const Expr& replacement) -> Expr;

/// Recursively rebuild an expression and apply a rule from leaves to root.
auto rewrite_bottom_up(const Expr& expression, const RewriteRule& rule) -> Expr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
