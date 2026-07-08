/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#include <symengine/add.h>
#include <symengine/basic-inl.h>
#include <symengine/basic.h>
#include <symengine/dict.h>
#include <symengine/eval_double.h>
#include <symengine/integer.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/rational.h>
#include <symengine/subs.h>
#include <symengine/symbol.h>
#include <symengine/symengine_casts.h>
#include <symengine/symengine_rcp.h>
#include <symengine/visitor.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"

namespace tit::sph {
namespace {

namespace se = SymEngine;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Symbolic math
//

/// Symbolic expression.
class Expr final {
public:

  /// Construct from a raw SymEngine expression.
  explicit(false) Expr(se::RCP<const se::Basic> basic)
      : base_{std::move(basic)} {}

  /// Construct an integer literal.
  explicit(false) Expr(int value) : base_{se::integer(value)} {}

  /// Underlying SymEngine expression.
  auto base() const -> const se::RCP<const se::Basic>& {
    return base_;
  }

  /// Expanded copy of this expression.
  auto expand() const -> Expr {
    return se::expand(base_);
  }

  /// Is this expression the integer zero?
  auto is_zero() const -> bool {
    return se::eq(*base_, *se::integer(0));
  }

  /// Numeric value of this expression.
  auto as_double() const -> double {
    return se::eval_double(*base_);
  }

  /// Does this expression contain the given symbol?
  auto uses(const Expr& sym) const -> bool {
    if (se::eq(*base_, *sym.base_)) return true;
    return std::ranges::any_of(base_->get_args(), [&sym](const Expr& expr) {
      return expr.uses(sym);
    });
  }

  /// Substitute `value` for `var` in this expression.
  auto subs(const Expr& var, const Expr& value) const -> Expr {
    return se::subs(base_, {{var.base_, value.base_}});
  }

  /// Arithmetic operators.
  /// @{
  friend auto operator-(const Expr& a) -> Expr {
    return se::neg(a.base_);
  }
  friend auto operator+(const Expr& a, const Expr& b) -> Expr {
    return se::add(a.base_, b.base_);
  }
  friend auto operator-(const Expr& a, const Expr& b) -> Expr {
    return se::sub(a.base_, b.base_);
  }
  friend auto operator*(const Expr& a, const Expr& b) -> Expr {
    return se::mul(a.base_, b.base_);
  }
  friend auto operator/(const Expr& a, const Expr& b) -> Expr {
    return se::div(a.base_, b.base_);
  }
  friend auto operator+=(Expr& a, const Expr& b) -> Expr& {
    a = a + b;
    return a;
  }
  /// @}

private:

  se::RCP<const se::Basic> base_;

}; // class Expr

/// Raise `base` to a power.
auto pow(const Expr& base, const Expr& exp) -> Expr {
  return se::pow(base.base(), exp.base());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Univariate view of an expanded expression.
class Poly final {
public:

  /// Expand `e` and extract its coefficients in `var`.
  Poly(const Expr& e, const Expr& var) {
    const auto expanded = e.expand();
    for (int p = 0; p <= max_degree_; ++p) {
      Expr c{se::coeff(*expanded.base(), *var.base(), *se::integer(p))};
      if (!c.is_zero()) {
        coeffs_.emplace(p, std::move(c));
        degree_ = p;
      }
    }
  }

  /// Degree of the polynomial.
  auto degree() const -> int {
    return degree_;
  }

  /// Coefficient of `var^power`, or zero if absent.
  auto operator[](int power) const -> Expr {
    const auto iter = coeffs_.find(power);
    return iter != coeffs_.end() ? iter->second : Expr{0};
  }

  /// Non-zero `(power, coefficient)` terms, in ascending order of power.
  auto terms() const -> const std::map<int, Expr>& {
    return coeffs_;
  }

private:

  // Maximum polynomial degree we ever encounter.
  static constexpr int max_degree_ = 32;

  std::map<int, Expr> coeffs_;
  int degree_ = 0;

}; // class Poly

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Optimizations
//

/// An expression with common subexpressions factored into named bindings.
class OptExpr final {
public:

  /// Eliminate common subexpressions in `e`.
  explicit OptExpr(const Expr& e) : result_{run_cse(e, reps_)} {}

  /// Common-subexpression bindings, in evaluation order.
  auto reps() const -> const std::vector<std::pair<Expr, Expr>>& {
    return reps_;
  }

  /// Final result expression.
  auto result() const -> const Expr& {
    return result_;
  }

  /// Does the optimized expression reference the given symbol?
  auto uses(const Expr& sym) const -> bool {
    if (result_.uses(sym)) return true;
    return std::ranges::any_of(reps_, [&sym](const auto& rep) {
      return rep.second.uses(sym);
    });
  }

private:

  // Perform common-subexpression elimination, filling `reps` and returning the
  // reduced result.
  static auto run_cse(const Expr& e, std::vector<std::pair<Expr, Expr>>& reps)
      -> Expr {
    se::vec_pair se_reps;
    se::vec_basic out;
    se::cse(se_reps, out, {e.base()});
    for (const auto& [sym, val] : se_reps) reps.emplace_back(sym, val);
    return Expr{out[0]};
  }

  std::vector<std::pair<Expr, Expr>> reps_;
  Expr result_;

}; // class OptExpr

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Rewrite the expression in multivariate Horner form.
auto horner(const Expr& e, std::span<const Expr> vars)
    -> Expr { // NOLINT(*-no-recursion)
  if (vars.empty()) return e;
  const auto& var = vars.front();
  const auto remaining_vars = vars.subspan(1);
  const Poly poly{e, var};
  const auto deg = poly.degree();
  auto result = horner(poly[deg], remaining_vars);
  for (int p = deg - 1; p >= 0; --p) {
    result = horner(poly[p], remaining_vars) + var * result;
  }
  return result;
}

/// Rewrite the expression in multivariate Horner form.
auto horner(const Expr& e, std::initializer_list<Expr> vars) -> Expr {
  return horner(e, std::span{vars});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Rewrite `sym^n` in `expr` as powers of `squared` (`sym^2`) times an optional
// leftover `sym`, so the generated code can reuse a precomputed square.
auto power_reduce(const se::RCP<const se::Basic>& expr,
                  const se::RCP<const se::Basic>& sym,
                  const se::RCP<const se::Basic>& squared)
    -> se::RCP<const se::Basic> {
  if (se::is_a<se::Pow>(*expr)) {
    const auto& p = se::down_cast<const se::Pow&>(*expr);
    if (se::eq(*p.get_base(), *sym) && se::is_a<se::Integer>(*p.get_exp())) {
      const auto n = se::down_cast<const se::Integer&>(*p.get_exp()).as_int();
      if (n >= 2) {
        auto reduced = se::pow(squared, se::integer(n / 2));
        if (n % 2 != 0) reduced = se::mul(reduced, sym);
        return reduced;
      }
    }
  }
  se::vec_basic new_args;
  bool changed = false;
  for (const auto& arg : expr->get_args()) {
    auto new_arg = power_reduce(arg, sym, squared);
    changed = changed || new_arg != arg;
    new_args.emplace_back(std::move(new_arg));
  }
  if (!changed) return expr;
  if (se::is_a<se::Add>(*expr)) return se::add(new_args);
  if (se::is_a<se::Mul>(*expr)) return se::mul(new_args);
  if (se::is_a<se::Pow>(*expr)) return se::pow(new_args[0], new_args[1]);
  return expr;
}

/// Rewrite `sym^n` as powers of `squared`, wrapped for `Expr`.
auto power_reduce(const Expr& e, const Expr& sym, const Expr& squared) -> Expr {
  return Expr{power_reduce(e.base(), sym.base(), squared.base())};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Kernel math
//

const Expr q{se::symbol("q")};         ///< Normalized radius.
const Expr z{se::symbol("z")};         ///< Along-edge coordinate.
const Expr eta{se::symbol("eta")};     ///< Wall distance.
const Expr rho{se::symbol("rho")};     ///< Radius, `rho^2 = z^2 + eta^2`.
const Expr delta{se::symbol("delta")}; ///< Edge offset.
const Expr beta{se::symbol("beta")};   ///< `beta^2 = eta^2 + delta^2`.
const Expr A{se::symbol("A")};         ///< `atan2` primitive.
const Expr B{se::symbol("B")};         ///< `atan2` primitive (line).
const Expr L{se::symbol("L")};         ///< `asinh` primitive.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// `J_p`: primitive of `rho^p` along a segment, with `rho^2 = z^2 + eta^2`.
auto j_expr(int power) -> Expr {
  if (power == 0) return z;
  if (power == 1) return (z * rho + pow(eta, 2) * L) / 2;
  return (z * pow(rho, power) + power * pow(eta, 2) * j_expr(power - 2)) /
         (power + 1);
}

/// `J_p` for the triangle edge, with `rho^2 = z^2 + beta^2`.
auto j_line_expr(int power) -> Expr {
  if (power == 0) return z;
  if (power == 1) return (z * rho + pow(beta, 2) * L) / 2;
  return (z * pow(rho, power) + power * pow(beta, 2) * j_line_expr(power - 2)) /
         (power + 1);
}

/// `K_p`: edge primitive built on top of the line integrals `J`.
auto k_line_expr(int power) -> Expr {
  if (power == 0) return A;
  if (power == 1) return delta * L + eta * B;
  return delta * j_line_expr(power - 2) + pow(eta, 2) * k_line_expr(power - 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A single piece of a piecewise-polynomial kernel support: the weight
/// polynomial `w(q)` valid for `q < cutoff`.
class Segment final {
public:

  /// Construct a support piece from its cutoff and weight polynomial.
  Segment(Expr cutoff, Expr weight)
      : cutoff_{std::move(cutoff)}, weight_{std::move(weight)} {}

  /// Support cutoff of this piece.
  auto cutoff() const -> Expr {
    return cutoff_;
  }

  /// Weight polynomial `w(q)`.
  auto value() const -> Expr {
    return weight_;
  }

  /// Radial derivative `w'(q)`.
  auto deriv() const -> Expr {
    const Poly poly{weight_, q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) {
      if (p >= 1) result += c * p * pow(q, p - 1);
    }
    return horner(result.subs(q, q + cutoff_), {q}).subs(q, q - cutoff_);
  }

  /// Tail moment of order `dim`:
  /// integral of `q^(dim-1) w(q)` from `q` to cutoff.
  auto tail_moment(int dim) const -> Expr {
    const Poly poly{weight_, q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) result += c * pow(q, p) / (dim + p);
    result = horner(result, {q});
    return pow(cutoff_, dim) * result.subs(q, cutoff_) - pow(q, dim) * result;
  }

  /// Kernel-flux primitive over a clipped 2D segment.
  auto flux_primitive() const -> OptExpr {
    const Poly poly{weight_, q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) result += c * j_expr(p);
    return optimize_radial_(result);
  }

  /// Antigradient-flux primitive over a clipped 2D segment.
  auto antigrad_flux_primitive() const -> OptExpr {
    const Poly poly{tail_moment(2), q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) {
      if (p == 0) result += c * A;
      else if (p == 1) result += c * eta * L;
      else result += c * eta * j_expr(p - 2);
    }
    return optimize_radial_(result);
  }

  /// Kernel-flux line primitive over a triangle edge.
  auto flux_line_primitive() const -> OptExpr {
    const Poly poly{flux_moment_(rho), rho};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) result += c * k_line_expr(p);
    return optimize_line_(result);
  }

  /// Antigradient-flux line primitive over a triangle edge.
  auto antigrad_flux_line_primitive() const -> OptExpr {
    const Poly poly{tail_moment(3), q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) {
      if (p == 0) {
        result += c * (k_line_expr(0) - B);
      } else {
        result += c *
                  (eta * k_line_expr(p - 1) - pow(eta, p) * k_line_expr(0)) /
                  (p - 1);
      }
    }
    return optimize_line_(result);
  }

  /// Kernel-flux sector primitive (angular part) over a triangle.
  auto flux_sector() const -> OptExpr {
    return optimize_sector_(flux_moment_(cutoff_));
  }

  /// Antigradient-flux sector primitive (angular part) over a triangle.
  auto antigrad_flux_sector() const -> OptExpr {
    const Poly poly{tail_moment(3), q};
    Expr result{0};
    for (const auto& [p, c] : poly.terms()) {
      if (p == 0) {
        result += c * (1 - eta / cutoff_);
      } else {
        result += c * (eta * pow(cutoff_, p - 1) - pow(eta, p)) / (p - 1);
      }
    }
    return optimize_sector_(result);
  }

private:

  // Moment of `w(q)` with `upper^(p+2)` bounds (2D flux, `rho`-parameterized).
  auto flux_moment_(const Expr& upper) const -> Expr {
    const Poly poly{weight_, q};
    Expr result = 0;
    for (const auto& [p, c] : poly.terms()) {
      const int e = p + 2;
      result += c * (pow(upper, e) - pow(eta, e)) / e;
    }
    return result;
  }

  // Optimize a radial primitive: reduce `rho`, Horner in `z` and `eta`, then
  // do common-subexpression elimination.
  static auto optimize_radial_(const Expr& e) -> OptExpr {
    const auto rho_reduced = power_reduce(e, rho, pow(z, 2) + pow(eta, 2));
    return OptExpr{horner(rho_reduced, {z, eta})};
  }

  // Optimize an edge primitive: reduce `rho` and `beta`, Horner, then
  // do common-subexpression elimination.
  static auto optimize_line_(const Expr& e) -> OptExpr {
    const auto rho_reduced =
        power_reduce(e, rho, pow(z, 2) + pow(eta, 2) + pow(delta, 2));
    const auto beta_reduced =
        power_reduce(rho_reduced, beta, pow(eta, 2) + pow(delta, 2));
    return OptExpr{horner(beta_reduced, {z, delta, eta})};
  }

  // Optimize a sector primitive: Horner in `eta`, then CSE.
  static auto optimize_sector_(const Expr& e) -> OptExpr {
    return OptExpr{horner(e, {eta})};
  }

  Expr cutoff_;
  Expr weight_;

}; // class Segment

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Smoothing kernel.
class Kernel final {
public:

  /// Construct a kernel from its name and support pieces.
  Kernel(std::string name, std::vector<Segment> segments)
      : name_{std::move(name)}, segments_{std::move(segments)} {}

  /// Kernel name.
  auto name() const -> const std::string& {
    return name_;
  }

  /// Support pieces.
  auto segments() const -> std::span<const Segment> {
    return segments_;
  }

  /// Support radius in units of the smoothing length (largest cutoff).
  auto unit_radius() const -> Expr {
    return std::ranges::max(segments_,
                            {},
                            [](const Segment& segment) {
                              return segment.cutoff().as_double();
                            })
        .cutoff();
  }

private:

  std::string name_;
  std::vector<Segment> segments_;

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The full set of kernels to generate.
auto kernels() -> std::vector<Kernel> {
  return {
      {"CubicSplineKernel",
       {
           {2, Expr{1} / 4 * pow(2 - q, 3)},
           {1, -pow(1 - q, 3)},
       }},
      {"QuarticSplineKernel",
       {
           {Expr{5} / 2, pow(Expr{5} / 2 - q, 4)},
           {Expr{3} / 2, -5 * pow(Expr{3} / 2 - q, 4)},
           {Expr{1} / 2, 10 * pow(Expr{1} / 2 - q, 4)},
       }},
      {"QuinticSplineKernel",
       {
           {3, pow(3 - q, 5)},
           {2, -6 * pow(2 - q, 5)},
           {1, 15 * pow(1 - q, 5)},
       }},
      {"QuarticWendlandKernel",
       {
           {2, (1 + 2 * q) * pow(1 - q / 2, 4)},
       }},
      {"SixthOrderWendlandKernel",
       {
           {2, (1 + 3 * q + Expr{35} / 12 * pow(q, 2)) * pow(1 - q / 2, 6)},
       }},
      {"EighthOrderWendlandKernel",
       {
           {2,
            (1 + 4 * q + Expr{25} / 4 * pow(q, 2) + 4 * pow(q, 3)) *
                pow(1 - q / 2, 8)},
       }},
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// C++ expression printer
//

/// Render symbolic expression as C++ code.
auto to_cxx(const se::RCP<const se::Basic>& expr)
    -> std::string { // NOLINT(*-no-recursion)
  if (se::is_a<se::Integer>(*expr)) {
    const auto n = se::down_cast<const se::Integer&>(*expr).as_int();
    return std::format("Num{{{}}}", n);
  }
  if (se::is_a<se::Rational>(*expr)) {
    const auto& r = se::down_cast<const se::Rational&>(*expr);
    auto num = r.get_num()->as_int();
    auto den = r.get_den()->as_int();
    if (den < 0) {
      num = -num;
      den = -den;
    }
    return std::format("Num{{{}.0 / {}.0}}", num, den);
  }
  if (se::is_a<se::Symbol>(*expr)) {
    return se::down_cast<const se::Symbol&>(*expr).get_name();
  }
  if (se::is_a<se::Add>(*expr)) {
    const auto args = expr->get_args();
    if (args.empty()) return "Num{0.0}";
    std::vector<std::string> parts;
    parts.reserve(args.size());
    for (const auto& arg : args) parts.emplace_back(to_cxx(arg));
    return std::format("({})", str_join(parts, " + "));
  }
  if (se::is_a<se::Mul>(*expr)) {
    se::RCP<const se::Basic> coeff = se::integer(1);
    std::vector<std::string> parts;
    for (const auto& arg : expr->get_args()) {
      if (se::is_a<se::Integer>(*arg) || se::is_a<se::Rational>(*arg)) {
        coeff = se::mul(coeff, arg);
      } else {
        parts.emplace_back(to_cxx(arg));
      }
    }
    if (se::eq(*coeff, *se::integer(-1))) {
      if (parts.empty()) return "Num{-1.0}";
      parts.front() = "-" + parts.front();
    } else if (!se::eq(*coeff, *se::integer(1))) {
      parts.insert(parts.begin(), to_cxx(coeff));
    }
    if (parts.empty()) return "Num{1.0}";
    return str_join(parts, " * ");
  }
  if (se::is_a<se::Pow>(*expr)) {
    const auto& p = se::down_cast<const se::Pow&>(*expr);
    const auto& base = p.get_base();
    const auto& exp = p.get_exp();
    if (se::is_a<se::Integer>(*exp)) {
      const auto n = se::down_cast<const se::Integer&>(*exp).as_int();
      if (n == -1) return std::format("inverse({})", to_cxx(base));
      if (n < -1) return std::format("inverse(pow({}, {}))", to_cxx(base), -n);
      return std::format("pow({}, {})", to_cxx(base), n);
    }
    return std::format("pow({}, {})", to_cxx(base), to_cxx(exp));
  }
  TIT_THROW("Cannot translate expression to C++.");
}

/// Render symbolic expression as C++ code.
auto to_cxx(const Expr& e) -> std::string {
  return to_cxx(e.base());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A support term guarded by its cutoff: `(q < cutoff ? expr : Num{0})`.
auto truncated(const Segment& segment, const Expr& e) -> std::string {
  return std::format("(q < {} ? {} : Num{{0}})",
                     to_cxx(segment.cutoff()),
                     to_cxx(e));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Join truncated support terms into a single returned sum expression.
auto join_terms(const std::vector<std::string>& terms) -> std::string {
  return terms.empty() ? "Num{0.0}" : str_join(terms, " + ");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Generated helper functions
//

/// Fully-qualified `<Num>`-instantiated name of a per-segment helper.
auto helper_ref(const Kernel& kernel,
                std::string_view name,
                std::ptrdiff_t index) -> std::string {
  return std::format("&impl::{}_gen::{}_{}<Num>", kernel.name(), name, index);
}

/// Emit a templated helper function from an optimized expression.
void emit_helper(std::ostream& os,
                 const std::string& name,
                 std::initializer_list<Expr> params,
                 const OptExpr& opt) {
  std::println(os, "template<class Num>");
  const auto head = std::format("constexpr auto {}(", name);
  const std::string cont(head.size(), ' ');
  std::print(os, "{}", head);
  for (const auto& [i, param] : std::views::enumerate(params)) {
    if (i != 0) std::print(os, ",\n{}", cont);
    if (!opt.uses(param)) std::print(os, "[[maybe_unused]] ");
    std::print(os, "Num {}", to_cxx(param));
  }
  std::println(os, ") noexcept -> Num {{");
  for (const auto& [sym, val] : opt.reps()) {
    std::println(os, "  const auto {} = {};", to_cxx(sym), to_cxx(val));
  }
  std::println(os, "  return {};", to_cxx(opt.result()));
  std::println(os, "}}");
  std::println(os);
}

/// Emit the `impl::<name>_gen` namespace of per-segment helpers.
void emit_helpers(std::ostream& os, const Kernel& kernel) {
  std::println(os, "namespace impl::{}_gen {{", kernel.name());
  std::println(os);
  for (const auto& [i, segment] : std::views::enumerate(kernel.segments())) {
    emit_helper(os,
                std::format("unit_flux_{}", i),
                {eta, z, rho, A, L},
                segment.flux_primitive());
    emit_helper(os,
                std::format("unit_antigrad_flux_{}", i),
                {eta, z, rho, A, L},
                segment.antigrad_flux_primitive());
    emit_helper(os,
                std::format("unit_flux_line_{}", i),
                {eta, delta, z, rho, A, B, L},
                segment.flux_line_primitive());
    emit_helper(os,
                std::format("unit_antigrad_flux_line_{}", i),
                {eta, delta, z, rho, A, B, L},
                segment.antigrad_flux_line_primitive());
    emit_helper(os,
                std::format("unit_flux_sector_{}", i),
                {eta},
                segment.flux_sector());
    emit_helper(os,
                std::format("unit_antigrad_flux_sector_{}", i),
                {eta},
                segment.antigrad_flux_sector());
  }
  std::println(os, "}} // namespace impl::{}_gen", kernel.name());
  std::println(os);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Generated kernel method definitions
//

void emit_radius(std::ostream& os, const Kernel& kernel) {
  std::println(os, "template<>");
  std::println(os, "template<class Num>");
  std::println(os,
               "consteval auto {}::unit_radius() noexcept -> Num {{",
               kernel.name());
  std::println(os, "  return {};", to_cxx(kernel.unit_radius()));
  std::println(os, "}}");
}

void emit_value(std::ostream& os, const Kernel& kernel) {
  std::vector<std::string> terms;
  for (const auto& segment : kernel.segments()) {
    terms.emplace_back(truncated(segment, segment.value()));
  }
  std::println(os, "template<>");
  std::println(os, "template<class Num>");
  std::println(os,
               "constexpr auto {}::unit_value(Num q) noexcept -> Num {{",
               kernel.name());
  std::println(os, "  return {};", join_terms(terms));
  std::println(os, "}}");
}

void emit_deriv(std::ostream& os, const Kernel& kernel) {
  std::vector<std::string> terms;
  for (const auto& segment : kernel.segments()) {
    terms.emplace_back(truncated(segment, segment.deriv()));
  }
  std::println(os, "template<>");
  std::println(os, "template<class Num>");
  std::println(os,
               "constexpr auto {}::unit_deriv(Num q) noexcept -> Num {{",
               kernel.name());
  std::println(os, "  return {};", join_terms(terms));
  std::println(os, "}}");
}

void emit_antideriv_moment(std::ostream& os, const Kernel& kernel) {
  const auto make_terms = [&kernel](int dim) {
    std::vector<std::string> terms;
    for (const auto& segment : kernel.segments()) {
      terms.emplace_back(truncated(segment, segment.tail_moment(dim)));
    }
    return terms;
  };
  std::println(os, "template<>");
  std::println(os, "template<std::size_t Dim, class Num>");
  std::println(
      os,
      "constexpr auto {}::unit_antideriv_moment(Num q) noexcept -> Num {{",
      kernel.name());
  std::println(os, "  if constexpr (Dim == 1) {{");
  std::println(os, "    return {};", join_terms(make_terms(1)));
  std::println(os, "  }} else if constexpr (Dim == 2) {{");
  std::println(os, "    return {};", join_terms(make_terms(2)));
  std::println(os, "  }} else if constexpr (Dim == 3) {{");
  std::println(os, "    return {};", join_terms(make_terms(3)));
  std::println(os, "  }} else {{");
  std::println(os, "    static_assert(false);");
  std::println(os, "  }}");
  std::println(os, "}}");
}

void emit_segment_flux(std::ostream& os,
                       const Kernel& kernel,
                       std::string_view method,
                       std::string_view helper_name) {
  std::vector<std::string> terms;
  for (const auto& [i, segment] : std::views::enumerate(kernel.segments())) {
    terms.emplace_back(std::format( //
        "unit_segment_integral({}, eta, z_min, z_max, {})",
        to_cxx(segment.cutoff()),
        helper_ref(kernel, helper_name, i)));
  }
  std::println(os, "template<>");
  std::println(os, "template<class Num>");
  std::println(os, "constexpr auto {}::{}(", kernel.name(), method);
  std::println(os, "    Num eta,");
  std::println(os, "    Num z_min,");
  std::println(os, "    Num z_max) noexcept -> Num {{");
  std::println(os, "  return {};", join_terms(terms));
  std::println(os, "}}");
}

void emit_triangle_flux(std::ostream& os,
                        const Kernel& kernel,
                        std::string_view method,
                        std::string_view line_name,
                        std::string_view sector_name) {
  std::vector<std::string> terms;
  for (const auto& [i, segment] : std::views::enumerate(kernel.segments())) {
    terms.emplace_back(std::format( //
        "unit_triangle_integral({}, eta, a, b, c, {}, {})",
        to_cxx(segment.cutoff()),
        helper_ref(kernel, line_name, i),
        helper_ref(kernel, sector_name, i)));
  }
  std::println(os, "template<>");
  std::println(os, "template<class Num>");
  std::println(os, "constexpr auto {}::{}(", kernel.name(), method);
  std::println(os, "    Num eta,");
  std::println(os, "    const Vec<Num, 2>& a,");
  std::println(os, "    const Vec<Num, 2>& b,");
  std::println(os, "    const Vec<Num, 2>& c) noexcept -> Num {{");
  std::println(os, "  return {};", join_terms(terms));
  std::println(os, "}}");
}

/// Emit all method definitions for a kernel, in declaration order.
void emit_methods(std::ostream& os, const Kernel& kernel) {
  emit_radius(os, kernel);
  std::println(os);
  emit_value(os, kernel);
  std::println(os);
  emit_deriv(os, kernel);
  std::println(os);
  emit_antideriv_moment(os, kernel);
  std::println(os);
  emit_segment_flux(os, kernel, "unit_flux", "unit_flux");
  std::println(os);
  emit_triangle_flux(os,
                     kernel,
                     "unit_flux",
                     "unit_flux_line",
                     "unit_flux_sector");
  std::println(os);
  emit_segment_flux(os, kernel, "unit_antigrad_flux", "unit_antigrad_flux");
  std::println(os);
  emit_triangle_flux(os,
                     kernel,
                     "unit_antigrad_flux",
                     "unit_antigrad_flux_line",
                     "unit_antigrad_flux_sector");
  std::println(os);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// File assembly
//

void emit_header(std::ostream& os) {
  std::println(os, "// This file is auto-generated. Do not edit manually.");
  std::println(os, "#pragma once");
  std::println(os);
  std::println(os, "#include <cstddef>");
  std::println(os);
  std::println(os, R"(#include "tit/core/math.hpp")");
  std::println(os, R"(#include "tit/core/vec.hpp")");
  std::println(
      os,
      R"(#include "tit/sph/kernel.hpp" // NOLINT(misc-header-include-cycle))");
  std::println(os);
  std::println(os, "namespace tit::sph {{");
  std::println(os);
}

void emit_footer(std::ostream& os) {
  std::println(os, "}} // namespace tit::sph");
}

/// Generate the kernel definitions and write them to `path`.
void generate(const std::string& path) {
  std::ofstream out{path};
  TIT_ENSURE(out.good(), "Unable to open output file '{}'.", path);
  emit_header(out);
  for (const auto& kernel : kernels()) {
    emit_helpers(out, kernel);
    emit_methods(out, kernel);
  }
  emit_footer(out);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph

auto main(int argc, char** argv) -> int { // NOLINT(*-exception-escape)
  try {
    tit::sph::generate(std::span{argv, argv + argc}.at(1));
  } catch (const std::exception& e) {
    std::println(std::cerr, "Kernel generator error: {}", e.what());
    return EXIT_FAILURE;
  } catch (...) {
    std::println(std::cerr, "Unknown error.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
