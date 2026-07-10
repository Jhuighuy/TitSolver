/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "tit/sym/rational.hpp"

namespace tit::sym {

namespace impl {
struct ExprAccess;
struct ExprNode;
struct ExprState;
} // namespace impl

class Context;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Kind of a symbolic expression node.
enum class ExprKind : std::uint8_t {
  rational,
  symbol,
  add,
  mul,
  pow,
};

/// Immutable canonical symbolic expression.
class Expr final {
public:

  Expr(const Expr&) = default;
  Expr(Expr&&) noexcept = default;
  auto operator=(const Expr&) -> Expr& = default;
  auto operator=(Expr&&) noexcept -> Expr& = default;
  ~Expr() = default;

  /// Context owning this expression.
  auto context() const -> Context;

  /// Node kind.
  auto kind() const noexcept -> ExprKind;

  /// Is this expression a rational value?
  auto is_rational() const noexcept -> bool;

  /// Rational value.
  auto as_rational() const -> const Rational&;

  /// Is this expression an integer value?
  auto is_integer() const noexcept -> bool;

  /// Integer value.
  auto as_integer() const -> Rational::value_type;

  /// Is this expression zero?
  auto is_zero() const noexcept -> bool;

  /// Is this expression one?
  auto is_one() const noexcept -> bool;

  /// Is this expression a symbol?
  auto is_symbol() const noexcept -> bool;

  /// Symbol name.
  auto symbol_name() const -> std::string_view;

  /// Number of child expressions.
  auto arity() const noexcept -> std::size_t;

  /// Child expression.
  auto operator[](std::size_t index) const -> Expr;

  /// Structural hash value.
  auto hash() const noexcept -> std::size_t;

  /// Arithmetic operators.
  /// @{
  friend auto operator-(const Expr& value) -> Expr;
  friend auto operator+(const Expr& left, const Expr& right) -> Expr;
  friend auto operator-(const Expr& left, const Expr& right) -> Expr;
  friend auto operator*(const Expr& left, const Expr& right) -> Expr;
  friend auto operator/(const Expr& left, const Expr& right) -> Expr;
  friend auto operator+(const Expr& left, Rational::value_type right) -> Expr;
  friend auto operator+(Rational::value_type left, const Expr& right) -> Expr;
  friend auto operator-(const Expr& left, Rational::value_type right) -> Expr;
  friend auto operator-(Rational::value_type left, const Expr& right) -> Expr;
  friend auto operator*(const Expr& left, Rational::value_type right) -> Expr;
  friend auto operator*(Rational::value_type left, const Expr& right) -> Expr;
  friend auto operator/(const Expr& left, Rational::value_type right) -> Expr;
  friend auto operator/(Rational::value_type left, const Expr& right) -> Expr;
  auto operator+=(const Expr& other) -> Expr&;
  auto operator-=(const Expr& other) -> Expr&;
  auto operator*=(const Expr& other) -> Expr&;
  auto operator/=(const Expr& other) -> Expr&;
  /// @}

  /// Structural comparison.
  /// @{
  friend auto operator==(const Expr& left, const Expr& right) noexcept -> bool;
  friend auto operator<=>(const Expr& left, const Expr& right) noexcept
      -> std::strong_ordering;
  /// @}

private:

  Expr(std::shared_ptr<impl::ExprState> state,
       const impl::ExprNode* node) noexcept;

  std::shared_ptr<impl::ExprState> state_;
  const impl::ExprNode* node_;

  friend struct impl::ExprAccess;

}; // class Expr

/// Expression construction context.
class Context final {
public:

  /// Construct an empty context.
  Context();

  /// Construct an integer expression.
  auto integer(Rational::value_type value) const -> Expr;

  /// Construct a rational expression.
  /// @{
  auto rational(Rational value) const -> Expr;
  auto rational(Rational::value_type numerator,
                Rational::value_type denominator) const -> Expr;
  /// @}

  /// Construct or retrieve a named symbol.
  auto symbol(std::string name) const -> Expr;

  /// Do two handles refer to the same construction context?
  friend auto operator==(const Context&, const Context&) noexcept
      -> bool = default;

private:

  explicit Context(std::shared_ptr<impl::ExprState> state) noexcept;

  std::shared_ptr<impl::ExprState> state_;

  friend class Expr;
  friend struct impl::ExprAccess;

}; // class Context

/// Raise an expression to a power.
/// @{
auto pow(const Expr& base, const Expr& exponent) -> Expr;
auto pow(const Expr& base, Rational::value_type exponent) -> Expr;
/// @}

/// Rebuild an expression node with a new set of children.
auto rebuild(const Expr& expression, std::span<const Expr> args) -> Expr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sym

template<>
struct std::hash<tit::sym::Expr> {
  auto operator()(const tit::sym::Expr& expression) const noexcept
      -> std::size_t {
    return expression.hash();
  }
};
