/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/sym/optimize.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/sym/expression.hpp"
#include "tit/sym/rational.hpp"
#include "tit/sym/rewrite.hpp"

namespace tit::sym {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto saturating_add(std::size_t left, std::size_t right) noexcept
    -> std::size_t {
  constexpr auto max = std::numeric_limits<std::size_t>::max();
  return right > max - left ? max : left + right;
}

auto saturating_mul(std::size_t left, std::size_t right) noexcept
    -> std::size_t {
  constexpr auto max = std::numeric_limits<std::size_t>::max();
  if (left == 0 || right == 0) return 0;
  return right > max / left ? max : left * right;
}

auto integer_power_cost(Rational::value_type exponent) noexcept -> std::size_t {
  const bool inverse = exponent < 0;
  auto remaining = inverse ? static_cast<std::uint64_t>(-(exponent + 1)) + 1 :
                             static_cast<std::uint64_t>(exponent);
  std::size_t result = inverse ? 4 : 0;
  while (remaining > 1) {
    result = saturating_add(result, (remaining & 1U) != 0 ? 2 : 1);
    remaining >>= 1U;
  }
  return result;
}

auto local_operation_cost(const Expr& expression) noexcept -> std::size_t {
  switch (expression.kind()) {
    case ExprKind::rational:
    case ExprKind::symbol:   return 0;
    case ExprKind::add:
    case ExprKind::mul:      return expression.arity() - 1;
    case ExprKind::pow:
      return expression[1].is_integer() ?
                 integer_power_cost(expression[1].as_integer()) :
                 16;
  }
  TIT_ALWAYS_ASSERT(false, "Unknown symbolic expression kind.");
  std::unreachable();
}

void collect_symbol_names(const Expr& expression,
                          std::unordered_set<std::string>& names) {
  std::unordered_set<Expr> visited;
  const auto visit = [&names, &visited](this auto&& self,
                                        const Expr& current) -> void {
    if (!visited.emplace(current).second) return;
    if (current.is_symbol()) names.emplace(current.symbol_name());
    for (std::size_t i = 0; i < current.arity(); ++i) self(current[i]);
  };
  visit(expression);
}

class TempSymbols final {
public:

  TempSymbols(const OptimizedExpr& expression, std::string prefix)
      : context_{expression.result().context()}, prefix_{std::move(prefix)} {
    collect_symbol_names(expression.result(), names_);
    for (const auto& binding : expression.bindings()) {
      collect_symbol_names(binding.symbol, names_);
      collect_symbol_names(binding.value, names_);
    }
  }

  TempSymbols(const Expr& expression, std::string prefix)
      : context_{expression.context()}, prefix_{std::move(prefix)} {
    collect_symbol_names(expression, names_);
  }

  auto next() -> Expr {
    while (true) {
      auto name = prefix_ + std::to_string(next_index_++);
      if (names_.emplace(name).second) return context_.symbol(std::move(name));
    }
  }

private:

  Context context_;
  std::string prefix_;
  std::size_t next_index_ = 0;
  std::unordered_set<std::string> names_;

}; // class TempSymbols

void count_occurrences(const Expr& expression,
                       std::unordered_map<Expr, std::size_t>& counts) {
  auto& count = counts.try_emplace(expression, 0).first->second;
  count = saturating_add(count, 1);
  for (std::size_t i = 0; i < expression.arity(); ++i) {
    count_occurrences(expression[i], counts);
  }
}

auto best_candidate(const std::vector<ExpressionBinding>& bindings,
                    const Expr& result) -> std::optional<Expr> {
  std::unordered_map<Expr, std::size_t> counts;
  count_occurrences(result, counts);
  for (const auto& binding : bindings) count_occurrences(binding.value, counts);

  std::optional<Expr> best;
  std::size_t best_saving = 0;
  for (const auto& [expression, count] : counts) {
    if (count < 2 || expression.is_rational() || expression.is_symbol()) {
      continue;
    }
    const auto cost = expression_cost(expression);
    const auto saving = saturating_mul(count - 1, cost.operations);
    if (saving > best_saving || (saving == best_saving && saving != 0 &&
                                 (!best || expression < *best))) {
      best = expression;
      best_saving = saving;
    }
  }
  return best;
}

auto ordered_bindings(std::vector<ExpressionBinding> bindings)
    -> std::vector<ExpressionBinding> {
  enum class VisitState : std::uint8_t { unseen, active, done };
  std::vector states(bindings.size(), VisitState::unseen);
  std::vector<ExpressionBinding> result;
  result.reserve(bindings.size());
  const auto visit = [&bindings, &states, &result](this auto&& self,
                                                   std::size_t index) -> void {
    if (states[index] == VisitState::done) return;
    TIT_ALWAYS_ASSERT(states[index] != VisitState::active,
                      "Cyclic symbolic expression bindings.");
    states[index] = VisitState::active;
    const auto& value = bindings[index].value;
    for (std::size_t dependency = 0; dependency < bindings.size();
         ++dependency) {
      if (dependency != index && contains(value, bindings[dependency].symbol)) {
        self(dependency);
      }
    }
    states[index] = VisitState::done;
    result.push_back(std::move(bindings[index]));
  };
  for (std::size_t i = 0; i < bindings.size(); ++i) visit(i);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto expression_cost(const Expr& expression) -> ExpressionCost {
  std::unordered_map<Expr, ExpressionCost> memo;
  const auto visit = [&memo](this auto&& self,
                             const Expr& current) -> ExpressionCost {
    if (const auto iter = memo.find(current); iter != memo.end()) {
      return iter->second;
    }
    ExpressionCost result{.operations = local_operation_cost(current),
                          .nodes = 1};
    for (std::size_t i = 0; i < current.arity(); ++i) {
      const auto child = self(current[i]);
      result.operations = saturating_add(result.operations, child.operations);
      result.nodes = saturating_add(result.nodes, child.nodes);
    }
    memo.emplace(current, result);
    return result;
  };
  return visit(expression);
}

OptimizedExpr::OptimizedExpr(std::vector<ExpressionBinding> bindings,
                             Expr result)
    : bindings_{std::move(bindings)}, result_{std::move(result)} {
  const auto context = result_.context();
  for (std::size_t i = 0; i < bindings_.size(); ++i) {
    TIT_ALWAYS_ASSERT(bindings_[i].symbol.is_symbol(),
                      "A symbolic expression binding must name a symbol.");
    TIT_ALWAYS_ASSERT(bindings_[i].symbol.context() == context &&
                          bindings_[i].value.context() == context,
                      "Symbolic expression binding contexts do not match.");
    for (std::size_t j = i; j < bindings_.size(); ++j) {
      TIT_ALWAYS_ASSERT(i == j || bindings_[i].symbol != bindings_[j].symbol,
                        "Symbolic expression binding names must be unique.");
      TIT_ALWAYS_ASSERT(!contains(bindings_[i].value, bindings_[j].symbol),
                        "Symbolic expression bindings are not ordered.");
    }
  }
}

auto OptimizedExpr::bindings() const noexcept
    -> const std::vector<ExpressionBinding>& {
  return bindings_;
}

auto OptimizedExpr::result() const noexcept -> const Expr& {
  return result_;
}

auto OptimizedExpr::uses(const Expr& symbol) const -> bool {
  if (std::ranges::any_of(bindings_, [&symbol](const auto& binding) {
        return binding.symbol == symbol;
      })) {
    return false;
  }
  if (contains(result_, symbol)) return true;
  return std::ranges::any_of(bindings_, [&symbol](const auto& binding) {
    return contains(binding.value, symbol);
  });
}

auto OptimizedExpr::cost() const -> ExpressionCost {
  auto result = expression_cost(result_);
  for (const auto& binding : bindings_) {
    const auto binding_cost = expression_cost(binding.value);
    result.operations =
        saturating_add(result.operations, binding_cost.operations);
    result.nodes = saturating_add(result.nodes, binding_cost.nodes);
  }
  return result;
}

auto optimize(const Expr& expression, std::string temp_prefix)
    -> OptimizedExpr {
  std::vector<ExpressionBinding> bindings;
  auto result = expression;
  TempSymbols temps{expression, std::move(temp_prefix)};
  while (const auto candidate = best_candidate(bindings, result)) {
    auto symbol = temps.next();
    for (auto& binding : bindings) {
      binding.value = substitute(binding.value, *candidate, symbol);
    }
    result = substitute(result, *candidate, symbol);
    bindings.push_back({.symbol = std::move(symbol), .value = *candidate});
  }
  return {ordered_bindings(std::move(bindings)), std::move(result)};
}

auto spill(const OptimizedExpr& expression,
           const SpillPredicate& predicate,
           std::string temp_prefix) -> OptimizedExpr {
  TempSymbols temps{expression, std::move(temp_prefix)};
  std::unordered_map<Expr, Expr> materialized;
  std::vector<ExpressionBinding> bindings;
  const auto visit =
      [&predicate, &temps, &materialized, &bindings](this auto&& self,
                                                     const Expr& current,
                                                     bool is_root) -> Expr {
    if (const auto iter = materialized.find(current);
        iter != materialized.end()) {
      return iter->second;
    }
    std::vector<Expr> args;
    args.reserve(current.arity());
    bool changed = false;
    for (std::size_t i = 0; i < current.arity(); ++i) {
      auto arg = self(current[i], false);
      changed = changed || arg != current[i];
      args.emplace_back(std::move(arg));
    }
    auto result = changed ? sym::rebuild(current, args) : current;
    if (!is_root && !result.is_rational() && !result.is_symbol() &&
        std::invoke(predicate, result, expression_cost(result))) {
      auto symbol = temps.next();
      bindings.push_back({.symbol = symbol, .value = result});
      materialized.emplace(result, symbol);
      return symbol;
    }
    return result;
  };

  for (const auto& binding : expression.bindings()) {
    auto value = visit(binding.value, true);
    bindings.push_back({.symbol = binding.symbol, .value = std::move(value)});
  }
  auto result = visit(expression.result(), true);
  return {ordered_bindings(std::move(bindings)), std::move(result)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
