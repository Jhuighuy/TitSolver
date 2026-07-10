/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/sym/polynomial.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/sym/expression.hpp"
#include "tit/sym/rewrite.hpp"

namespace tit::sym {
namespace {

using Coefficients = std::vector<Expr>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void trim(Coefficients& coefficients) {
  while (coefficients.size() > 1 && coefficients.back().is_zero()) {
    coefficients.pop_back();
  }
}

auto zero_coefficients(const Context& context, std::size_t size)
    -> Coefficients {
  return {size, context.integer(0)};
}

auto add(const Coefficients& left, const Coefficients& right) -> Coefficients {
  TIT_ALWAYS_ASSERT(left.front().context() == right.front().context(),
                    "Polynomial contexts do not match.");
  auto result = zero_coefficients(left.front().context(),
                                  std::max(left.size(), right.size()));
  for (std::size_t i = 0; i < left.size(); ++i) result[i] += left[i];
  for (std::size_t i = 0; i < right.size(); ++i) result[i] += right[i];
  trim(result);
  return result;
}

auto multiply(const Coefficients& left, const Coefficients& right)
    -> Coefficients {
  TIT_ALWAYS_ASSERT(left.front().context() == right.front().context(),
                    "Polynomial contexts do not match.");
  auto result =
      zero_coefficients(left.front().context(), left.size() + right.size() - 1);
  for (std::size_t i = 0; i < left.size(); ++i) {
    for (std::size_t j = 0; j < right.size(); ++j) {
      result[i + j] += left[i] * right[j];
    }
  }
  trim(result);
  return result;
}

auto power(Coefficients base, std::size_t exponent) -> Coefficients {
  auto result = Coefficients{base.front().context().integer(1)};
  while (exponent != 0) {
    if ((exponent & 1U) != 0) result = multiply(result, base);
    exponent >>= 1U;
    if (exponent != 0) base = multiply(base, base);
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Polynomial::Polynomial(std::vector<Expr> coefficients)
    : coefficients_{std::move(coefficients)} {
  TIT_ALWAYS_ASSERT(!coefficients_.empty(),
                    "A polynomial must contain a constant coefficient.");
}

auto Polynomial::degree() const noexcept -> std::size_t {
  return coefficients_.size() - 1;
}

auto Polynomial::operator[](std::size_t power) const -> Expr {
  if (power < coefficients_.size()) return coefficients_[power];
  return coefficients_.front().context().integer(0);
}

auto Polynomial::coefficients() const noexcept -> std::span<const Expr> {
  return coefficients_;
}

auto as_polynomial(const Expr& expression, const Expr& variable) -> Polynomial {
  TIT_ENSURE(variable.is_symbol(), "Polynomial variable must be a symbol.");
  TIT_ALWAYS_ASSERT(expression.context() == variable.context(),
                    "Polynomial contexts do not match.");
  const auto context = expression.context();
  std::unordered_map<Expr, Coefficients> memo;
  const auto visit =
      [&variable, &context, &memo](this auto&& self,
                                   const Expr& current) -> Coefficients {
    if (const auto iter = memo.find(current); iter != memo.end()) {
      return iter->second;
    }
    Coefficients result;
    if (current == variable) {
      result = {context.integer(0), context.integer(1)};
    } else if (!contains(current, variable)) {
      result = {current};
    } else if (current.kind() == ExprKind::add) {
      result = {context.integer(0)};
      for (std::size_t i = 0; i < current.arity(); ++i) {
        result = add(result, self(current[i]));
      }
    } else if (current.kind() == ExprKind::mul) {
      result = {context.integer(1)};
      for (std::size_t i = 0; i < current.arity(); ++i) {
        result = multiply(result, self(current[i]));
      }
    } else if (current.kind() == ExprKind::pow && current[1].is_integer()) {
      const auto exponent = current[1].as_integer();
      TIT_ENSURE(exponent >= 0,
                 "Polynomial variable occurs under a negative power.");
      TIT_ENSURE(std::in_range<std::size_t>(exponent),
                 "Polynomial degree is too large.");
      result = power(self(current[0]), static_cast<std::size_t>(exponent));
    } else {
      TIT_THROW("Expression is not polynomial in variable '{}'.",
                variable.symbol_name());
    }
    trim(result);
    memo.emplace(current, result);
    return result;
  };
  return Polynomial{visit(expression)};
}

auto horner(const Expr& expression, std::span<const Expr> variables)
    -> Expr { // NOLINT(*-no-recursion)
  if (variables.empty()) return expression;
  const auto& variable = variables.front();
  const auto remaining = variables.subspan(1);
  const auto polynomial = as_polynomial(expression, variable);
  auto result = horner(polynomial[polynomial.degree()], remaining);
  for (auto power = polynomial.degree(); power > 0; --power) {
    result = horner(polynomial[power - 1], remaining) + variable * result;
  }
  return result;
}

auto horner(const Expr& expression, std::initializer_list<Expr> variables)
    -> Expr {
  return horner(expression, std::span{variables});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
