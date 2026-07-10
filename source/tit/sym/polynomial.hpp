/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <initializer_list>
#include <span>
#include <vector>

#include "tit/sym/expression.hpp"

namespace tit::sym {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Univariate polynomial with symbolic coefficients.
class Polynomial final {
public:

  /// Polynomial degree. The zero polynomial has degree zero.
  auto degree() const noexcept -> std::size_t;

  /// Coefficient of `variable^power`, or zero if the power is absent.
  auto operator[](std::size_t power) const -> Expr;

  /// All coefficients in ascending power order.
  auto coefficients() const noexcept -> std::span<const Expr>;

private:

  explicit Polynomial(std::vector<Expr> coefficients);

  std::vector<Expr> coefficients_;

  friend auto as_polynomial(const Expr& expression, const Expr& variable)
      -> Polynomial;

}; // class Polynomial

/// Decompose an expression as a polynomial in `variable`.
auto as_polynomial(const Expr& expression, const Expr& variable) -> Polynomial;

/// Rewrite an expression in multivariate Horner form.
/// @{
auto horner(const Expr& expression, std::span<const Expr> variables) -> Expr;
auto horner(const Expr& expression, std::initializer_list<Expr> variables)
    -> Expr;
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym
