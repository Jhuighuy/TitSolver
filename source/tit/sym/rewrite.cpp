/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/sym/expression.hpp"
#include "tit/sym/rewrite.hpp"

namespace tit::sym {

auto contains(const Expr& expression, const Expr& subexpression) -> bool {
  std::unordered_map<Expr, bool> memo;
  const auto visit = [&subexpression, &memo](this auto&& self,
                                             const Expr& current) -> bool {
    if (current == subexpression) return true;
    if (const auto iter = memo.find(current); iter != memo.end()) {
      return iter->second;
    }
    bool result = false;
    for (std::size_t i = 0; i < current.arity() && !result; ++i) {
      result = self(current[i]);
    }
    memo.emplace(current, result);
    return result;
  };
  return visit(expression);
}

auto rewrite_bottom_up(const Expr& expression, const RewriteRule& rule)
    -> Expr {
  std::unordered_map<Expr, Expr> memo;
  const auto context = expression.context();
  const auto visit = [&rule, &memo, &context](this auto&& self,
                                              const Expr& current) -> Expr {
    if (const auto iter = memo.find(current); iter != memo.end()) {
      return iter->second;
    }
    std::vector<Expr> args;
    args.reserve(current.arity());
    bool changed = false;
    for (std::size_t i = 0; i < current.arity(); ++i) {
      auto arg = self(current[i]);
      changed = changed || arg != current[i];
      args.emplace_back(std::move(arg));
    }
    auto result = changed ? sym::rebuild(current, args) : current;
    if (auto replacement = std::invoke(rule, result)) {
      TIT_ALWAYS_ASSERT(replacement->context() == context,
                        "A rewrite changed the expression context.");
      result = std::move(*replacement);
    }
    memo.emplace(current, result);
    return result;
  };
  return visit(expression);
}

auto substitute(const Expr& expression,
                const Expr& subexpression,
                const Expr& replacement) -> Expr {
  TIT_ALWAYS_ASSERT(expression.context() == subexpression.context() &&
                        expression.context() == replacement.context(),
                    "A substitution changed the expression context.");
  return rewrite_bottom_up(
      expression,
      [&subexpression, &replacement](const Expr& current) -> RewriteResult {
        if (current == subexpression) return replacement;
        return std::nullopt;
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
