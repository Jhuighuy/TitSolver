/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/sym/expression.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/sym/rational.hpp"

namespace tit::sym {
namespace impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ExprNode final {
  ExprKind kind;
  Rational rational;
  std::string symbol;
  std::vector<const ExprNode*> args;
  std::size_t hash;
};

struct ExprState final {
  std::vector<std::unique_ptr<ExprNode>> nodes;
  std::unordered_multimap<std::size_t, const ExprNode*> nodes_by_hash;
};

struct ExprAccess final {
  static auto state(const Expr& expression)
      -> const std::shared_ptr<ExprState>& {
    return expression.state_;
  }

  static auto node(const Expr& expression) -> const ExprNode* {
    return expression.node_;
  }

  static auto make(std::shared_ptr<ExprState> state, const ExprNode* node)
      -> Expr {
    return Expr{std::move(state), node};
  }

  static auto state(const Context& context)
      -> const std::shared_ptr<ExprState>& {
    return context.state_;
  }

  static auto make_context(std::shared_ptr<ExprState> state) -> Context {
    return Context{std::move(state)};
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl
namespace {

using Node = impl::ExprNode;
using State = impl::ExprState;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto hash_combine(std::size_t seed, std::size_t value) noexcept -> std::size_t {
  return seed ^ (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
}

auto node_hash(ExprKind kind,
               Rational rational,
               const std::string& symbol,
               const std::vector<const Node*>& args) noexcept -> std::size_t {
  auto result = std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(kind));
  if (kind == ExprKind::rational) {
    result = hash_combine(result, rational.hash());
  } else if (kind == ExprKind::symbol) {
    result = hash_combine(result, std::hash<std::string>{}(symbol));
  } else {
    for (const auto* arg : args) result = hash_combine(result, arg->hash);
  }
  return result;
}

auto nodes_equal(const Node& left, const Node& right) noexcept -> bool {
  return left.kind == right.kind && left.rational == right.rational &&
         left.symbol == right.symbol && left.args == right.args;
}

auto compare_nodes(const Node* left, const Node* right) noexcept
    -> std::strong_ordering { // NOLINT(*-no-recursion)
  if (left == right) return std::strong_ordering::equal;
  if (left->kind != right->kind) return left->kind <=> right->kind;
  if (left->kind == ExprKind::rational) {
    return left->rational <=> right->rational;
  }
  if (left->kind == ExprKind::symbol) return left->symbol <=> right->symbol;
  if (left->args.size() != right->args.size()) {
    return left->args.size() <=> right->args.size();
  }
  for (const auto [left_arg, right_arg] :
       std::views::zip(left->args, right->args)) {
    if (const auto result = compare_nodes(left_arg, right_arg);
        result != std::strong_ordering::equal) {
      return result;
    }
  }
  return std::strong_ordering::equal;
}

struct NodeLess final {
  auto operator()(const Node* left, const Node* right) const noexcept -> bool {
    return compare_nodes(left, right) == std::strong_ordering::less;
  }
};

auto intern(const std::shared_ptr<State>& state,
            ExprKind kind,
            Rational rational = {},
            std::string symbol = {},
            std::vector<const Node*> args = {}) -> const Node* {
  const auto hash = node_hash(kind, rational, symbol, args);
  Node candidate{.kind = kind,
                 .rational = rational,
                 .symbol = std::move(symbol),
                 .args = std::move(args),
                 .hash = hash};
  const auto [first, last] = state->nodes_by_hash.equal_range(hash);
  for (auto iter = first; iter != last; ++iter) {
    if (nodes_equal(*iter->second, candidate)) return iter->second;
  }
  const auto* result =
      state->nodes.emplace_back(std::make_unique<Node>(std::move(candidate)))
          .get();
  state->nodes_by_hash.emplace(hash, result);
  return result;
}

auto make_rational(const std::shared_ptr<State>& state, Rational value)
    -> const Node* {
  return intern(state, ExprKind::rational, value);
}

auto integer_value(const Node* node) -> Rational::value_type {
  TIT_ENSURE(node->kind == ExprKind::rational && node->rational.is_integer(),
             "Expression is not an integer.");
  return node->rational.numerator();
}

auto checked_add(Rational::value_type left, Rational::value_type right)
    -> Rational::value_type {
  Rational::value_type result = 0;
  TIT_ENSURE(!__builtin_add_overflow(left, right, &result),
             "Integer exponent overflow.");
  return result;
}

auto checked_mul(Rational::value_type left, Rational::value_type right)
    -> Rational::value_type {
  Rational::value_type result = 0;
  TIT_ENSURE(!__builtin_mul_overflow(left, right, &result),
             "Integer exponent overflow.");
  return result;
}

auto make_pow(const std::shared_ptr<State>& state,
              const Node* base,
              const Node* exponent) -> const Node*;

auto make_mul(const std::shared_ptr<State>& state,
              const std::vector<const Node*>& args) -> const Node* {
  Rational coefficient{1};
  std::vector<const Node*> flattened;
  for (const auto* arg : args) {
    if (arg->kind == ExprKind::rational) {
      coefficient *= arg->rational;
    } else if (arg->kind == ExprKind::mul) {
      flattened.insert(flattened.end(), arg->args.begin(), arg->args.end());
    } else {
      flattened.push_back(arg);
    }
  }
  if (coefficient == Rational{0}) return make_rational(state, 0);

  std::map<const Node*, Rational::value_type, NodeLess> factors;
  for (const auto* factor : flattened) {
    if (factor->kind == ExprKind::rational) {
      coefficient *= factor->rational;
      continue;
    }
    const Node* base = factor;
    Rational::value_type exponent = 1;
    if (factor->kind == ExprKind::pow &&
        factor->args[1]->kind == ExprKind::rational &&
        factor->args[1]->rational.is_integer()) {
      base = factor->args[0];
      exponent = factor->args[1]->rational.numerator();
    }
    const auto [iter, inserted] = factors.try_emplace(base, exponent);
    if (!inserted) iter->second = checked_add(iter->second, exponent);
  }

  std::vector<const Node*> result_args;
  if (coefficient != Rational{1}) {
    result_args.push_back(make_rational(state, coefficient));
  }
  for (const auto& [base, exponent] : factors) {
    if (exponent == 0) continue;
    result_args.push_back(
        exponent == 1 ? base :
                        make_pow(state, base, make_rational(state, exponent)));
  }
  if (result_args.empty()) return make_rational(state, 1);
  if (result_args.size() == 1) return result_args.front();
  return intern(state, ExprKind::mul, {}, {}, std::move(result_args));
}

auto make_add(const std::shared_ptr<State>& state,
              const std::vector<const Node*>& args) -> const Node* {
  std::vector<const Node*> flattened;
  for (const auto* arg : args) {
    if (arg->kind == ExprKind::add) {
      flattened.insert(flattened.end(), arg->args.begin(), arg->args.end());
    } else {
      flattened.push_back(arg);
    }
  }

  Rational constant{0};
  std::map<const Node*, Rational, NodeLess> terms;
  for (const auto* term : flattened) {
    if (term->kind == ExprKind::rational) {
      constant += term->rational;
      continue;
    }
    Rational coefficient{1};
    const Node* base = term;
    if (term->kind == ExprKind::mul &&
        term->args.front()->kind == ExprKind::rational) {
      coefficient = term->args.front()->rational;
      const std::vector<const Node*> factors{term->args.begin() + 1,
                                             term->args.end()};
      base = make_mul(state, factors);
    }
    const auto [iter, inserted] = terms.try_emplace(base, coefficient);
    if (!inserted) iter->second += coefficient;
  }

  std::vector<const Node*> result_args;
  if (constant != Rational{0}) {
    result_args.push_back(make_rational(state, constant));
  }
  for (const auto& [base, coefficient] : terms) {
    if (coefficient == Rational{0}) continue;
    result_args.push_back(
        coefficient == Rational{1} ?
            base :
            make_mul(state, {make_rational(state, coefficient), base}));
  }
  if (result_args.empty()) return make_rational(state, 0);
  if (result_args.size() == 1) return result_args.front();
  return intern(state, ExprKind::add, {}, {}, std::move(result_args));
}

auto rational_pow(Rational base, Rational::value_type exponent) -> Rational {
  const bool inverse = exponent < 0;
  auto remaining = inverse ? static_cast<std::uint64_t>(-(exponent + 1)) + 1 :
                             static_cast<std::uint64_t>(exponent);
  Rational result{1};
  while (remaining != 0) {
    if ((remaining & 1U) != 0) result *= base;
    remaining >>= 1U;
    if (remaining != 0) base *= base;
  }
  return inverse ? Rational{1} / result : result;
}

auto make_pow(const std::shared_ptr<State>& state,
              const Node* base,
              const Node* exponent) -> const Node* {
  if (exponent->kind == ExprKind::rational && exponent->rational.is_integer()) {
    const auto value = exponent->rational.numerator();
    if (value == 0) return make_rational(state, 1);
    if (value == 1) return base;
    if (base->kind == ExprKind::rational) {
      return make_rational(state, rational_pow(base->rational, value));
    }
    if (base->kind == ExprKind::pow &&
        base->args[1]->kind == ExprKind::rational &&
        base->args[1]->rational.is_integer()) {
      return make_pow(
          state,
          base->args[0],
          make_rational(
              state,
              checked_mul(base->args[1]->rational.numerator(), value)));
    }
  }
  if (base->kind == ExprKind::rational && base->rational == Rational{1}) {
    return base;
  }
  return intern(state, ExprKind::pow, {}, {}, {base, exponent});
}

auto expression(const std::shared_ptr<State>& state, const Node* node) -> Expr {
  return impl::ExprAccess::make(state, node);
}

void ensure_same_context(const Expr& left, const Expr& right) {
  TIT_ALWAYS_ASSERT(impl::ExprAccess::state(left) ==
                        impl::ExprAccess::state(right),
                    "Symbolic expressions belong to different contexts.");
}

auto make_integer_like(const Expr& like, Rational::value_type value) -> Expr {
  const auto& state = impl::ExprAccess::state(like);
  return expression(state, make_rational(state, value));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Expr::Expr(std::shared_ptr<impl::ExprState> state,
           const impl::ExprNode* node) noexcept
    : state_{std::move(state)}, node_{node} {}

auto Expr::context() const -> Context {
  return impl::ExprAccess::make_context(state_);
}

auto Expr::kind() const noexcept -> ExprKind {
  return node_->kind;
}

auto Expr::is_rational() const noexcept -> bool {
  return kind() == ExprKind::rational;
}

auto Expr::as_rational() const -> const Rational& {
  TIT_ENSURE(is_rational(), "Expression is not rational.");
  return node_->rational;
}

auto Expr::is_integer() const noexcept -> bool {
  return is_rational() && node_->rational.is_integer();
}

auto Expr::as_integer() const -> Rational::value_type {
  return integer_value(node_);
}

auto Expr::is_zero() const noexcept -> bool {
  return is_rational() && node_->rational == Rational{0};
}

auto Expr::is_one() const noexcept -> bool {
  return is_rational() && node_->rational == Rational{1};
}

auto Expr::is_symbol() const noexcept -> bool {
  return kind() == ExprKind::symbol;
}

auto Expr::symbol_name() const -> std::string_view {
  TIT_ENSURE(is_symbol(), "Expression is not a symbol.");
  return node_->symbol;
}

auto Expr::arity() const noexcept -> std::size_t {
  return node_->args.size();
}

auto Expr::operator[](std::size_t index) const -> Expr {
  TIT_ENSURE(index < arity(), "Expression argument index is out of bounds.");
  return expression(state_, node_->args[index]);
}

auto Expr::hash() const noexcept -> std::size_t {
  return node_->hash;
}

Context::Context() : state_{std::make_shared<impl::ExprState>()} {}

Context::Context(std::shared_ptr<impl::ExprState> state) noexcept
    : state_{std::move(state)} {}

auto Context::integer(Rational::value_type value) const -> Expr {
  return expression(state_, make_rational(state_, value));
}

auto Context::rational(Rational value) const -> Expr {
  return expression(state_, make_rational(state_, value));
}

auto Context::rational(Rational::value_type numerator,
                       Rational::value_type denominator) const -> Expr {
  return rational(Rational{numerator, denominator});
}

auto Context::symbol(std::string name) const -> Expr {
  return expression(state_,
                    intern(state_, ExprKind::symbol, {}, std::move(name)));
}

auto operator-(const Expr& value) -> Expr {
  return make_integer_like(value, -1) * value;
}

auto operator+(const Expr& left, const Expr& right) -> Expr {
  ensure_same_context(left, right);
  const auto& state = impl::ExprAccess::state(left);
  return expression(
      state,
      make_add(state,
               {impl::ExprAccess::node(left), impl::ExprAccess::node(right)}));
}

auto operator-(const Expr& left, const Expr& right) -> Expr {
  return left + -right;
}

auto operator*(const Expr& left, const Expr& right) -> Expr {
  ensure_same_context(left, right);
  const auto& state = impl::ExprAccess::state(left);
  return expression(
      state,
      make_mul(state,
               {impl::ExprAccess::node(left), impl::ExprAccess::node(right)}));
}

auto operator/(const Expr& left, const Expr& right) -> Expr {
  return left * pow(right, -1);
}

auto operator+(const Expr& left, Rational::value_type right) -> Expr {
  return left + make_integer_like(left, right);
}

auto operator+(Rational::value_type left, const Expr& right) -> Expr {
  return make_integer_like(right, left) + right;
}

auto operator-(const Expr& left, Rational::value_type right) -> Expr {
  return left - make_integer_like(left, right);
}

auto operator-(Rational::value_type left, const Expr& right) -> Expr {
  return make_integer_like(right, left) - right;
}

auto operator*(const Expr& left, Rational::value_type right) -> Expr {
  return left * make_integer_like(left, right);
}

auto operator*(Rational::value_type left, const Expr& right) -> Expr {
  return make_integer_like(right, left) * right;
}

auto operator/(const Expr& left, Rational::value_type right) -> Expr {
  return left / make_integer_like(left, right);
}

auto operator/(Rational::value_type left, const Expr& right) -> Expr {
  return make_integer_like(right, left) / right;
}

auto Expr::operator+=(const Expr& other) -> Expr& {
  return *this = *this + other;
}

auto Expr::operator-=(const Expr& other) -> Expr& {
  return *this = *this - other;
}

auto Expr::operator*=(const Expr& other) -> Expr& {
  return *this = *this * other;
}

auto Expr::operator/=(const Expr& other) -> Expr& {
  return *this = *this / other;
}

auto pow(const Expr& base, const Expr& exponent) -> Expr {
  ensure_same_context(base, exponent);
  const auto& state = impl::ExprAccess::state(base);
  return expression(state,
                    make_pow(state,
                             impl::ExprAccess::node(base),
                             impl::ExprAccess::node(exponent)));
}

auto pow(const Expr& base, Rational::value_type exponent) -> Expr {
  return pow(base, make_integer_like(base, exponent));
}

auto rebuild(const Expr& expression, std::span<const Expr> args) -> Expr {
  TIT_ALWAYS_ASSERT(args.size() == expression.arity(),
                    "A rebuilt expression must preserve its arity.");
  const auto context = expression.context();
  switch (expression.kind()) {
    case ExprKind::rational:
    case ExprKind::symbol:   return expression;
    case ExprKind::add:      {
      auto result = context.integer(0);
      for (const auto& arg : args) result += arg;
      return result;
    }
    case ExprKind::mul: {
      auto result = context.integer(1);
      for (const auto& arg : args) result *= arg;
      return result;
    }
    case ExprKind::pow: return pow(args[0], args[1]);
  }
  TIT_ALWAYS_ASSERT(false, "Unknown symbolic expression kind.");
  std::unreachable();
}

auto operator==(const Expr& left, const Expr& right) noexcept -> bool {
  return compare_nodes(impl::ExprAccess::node(left),
                       impl::ExprAccess::node(right)) ==
         std::strong_ordering::equal;
}

auto operator<=>(const Expr& left, const Expr& right) noexcept
    -> std::strong_ordering {
  return compare_nodes(impl::ExprAccess::node(left),
                       impl::ExprAccess::node(right));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
