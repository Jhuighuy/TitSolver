/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/sym/expression.hpp"
#include "tit/sym/optimize.hpp"
#include "tit/sym/polynomial.hpp"
#include "tit/sym/rational.hpp"
#include "tit/sym/rewrite.hpp"

namespace tit::sph {
namespace {

using sym::Expr;
using sym::OptimizedExpr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Symbolic math
//

// Rewrite `sym^n` in `expr` as powers of `squared` (`sym^2`) times an optional
// leftover `sym`, so the generated code can reuse a precomputed square.
auto power_reduce(const Expr& expression,
                  const Expr& symbol,
                  const Expr& squared) -> Expr {
  return sym::rewrite_bottom_up(
      expression,
      [&symbol, &squared](const Expr& current) -> sym::RewriteResult {
        if (current.kind() != sym::ExprKind::pow || current[0] != symbol ||
            !current[1].is_integer()) {
          return std::nullopt;
        }
        const auto exponent = current[1].as_integer();
        if (exponent < 2) return std::nullopt;
        auto result = sym::pow(squared, exponent / 2);
        if (exponent % 2 != 0) result *= symbol;
        return result;
      });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Kernel math
//

const sym::Context expr_context;
const Expr q = expr_context.symbol("q");         ///< Normalized radius.
const Expr z = expr_context.symbol("z");         ///< Along-edge coordinate.
const Expr eta = expr_context.symbol("eta");     ///< Wall distance.
const Expr rho = expr_context.symbol("rho");     ///< `rho^2 = z^2 + eta^2`.
const Expr delta = expr_context.symbol("delta"); ///< Edge offset.
const Expr beta = expr_context.symbol("beta");   ///< `eta^2 + delta^2`.
const Expr A = expr_context.symbol("A");         ///< `atan2` primitive.
const Expr B = expr_context.symbol("B");         ///< `atan2` primitive (line).
const Expr L = expr_context.symbol("L");         ///< `asinh` primitive.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Render symbolic expression as C++ code.
auto to_cxx(const Expr& expression) -> std::string;

/// Generated-code cost, ordered by run-time work and then source complexity.
auto optimized_cost(const OptimizedExpr& expression)
    -> std::tuple<std::size_t, std::size_t, std::size_t> {
  std::size_t emitted_size = to_cxx(expression.result()).size();
  for (const auto& binding : expression.bindings()) {
    emitted_size += to_cxx(binding.symbol).size();
    emitted_size += to_cxx(binding.value).size();
  }
  return {expression.cost().operations,
          emitted_size,
          expression.bindings().size()};
}

/// Optimize the best of several algebraically equivalent expressions.
auto optimize_best(const std::vector<Expr>& candidates) -> OptimizedExpr {
  TIT_ALWAYS_ASSERT(!candidates.empty(), "No optimization candidates.");
  auto best = sym::optimize(candidates.front());
  for (const auto& candidate : std::span{candidates}.subspan(1)) {
    auto optimized = sym::optimize(candidate);
    if (optimized_cost(optimized) < optimized_cost(best)) {
      best = std::move(optimized);
    }
  }
  return sym::spill(best, [](const Expr& expression, sym::ExpressionCost cost) {
    return expression.kind() != sym::ExprKind::pow && cost.operations >= 8 &&
           to_cxx(expression).size() >= 80;
  });
}

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

  /// Construct a support piece with an integer cutoff.
  Segment(sym::Rational::value_type cutoff, Expr weight)
      : Segment{expr_context.integer(cutoff), std::move(weight)} {}

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
    const auto polynomial = sym::as_polynomial(weight_, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (p >= 1) result += c * p * pow(q, p - 1);
    }
    result = sym::substitute(result, q, q + cutoff_);
    result = sym::horner(result, {q});
    return sym::substitute(result, q, q - cutoff_);
  }

  /// Tail moment of order `dim`:
  /// integral of `q^(dim-1) w(q)` from `q` to cutoff.
  auto tail_moment(int dim) const -> Expr {
    const auto polynomial = sym::as_polynomial(weight_, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (!c.is_zero()) result += c * pow(q, p) / (dim + p);
    }
    result = sym::horner(result, {q});
    return pow(cutoff_, dim) * sym::substitute(result, q, cutoff_) -
           pow(q, dim) * result;
  }

  /// Kernel-flux primitive over a clipped 2D segment.
  auto flux_primitive() const -> OptimizedExpr {
    const auto polynomial = sym::as_polynomial(weight_, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (!c.is_zero()) result += c * j_expr(static_cast<int>(p));
    }
    return optimize_radial_(result);
  }

  /// Antigradient-flux primitive over a clipped 2D segment.
  auto antigrad_flux_primitive() const -> OptimizedExpr {
    const auto moment = tail_moment(2);
    const auto polynomial = sym::as_polynomial(moment, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (c.is_zero()) continue;
      if (p == 0) result += c * A;
      else if (p == 1) result += c * eta * L;
      else result += c * eta * j_expr(static_cast<int>(p - 2));
    }
    return optimize_radial_(result);
  }

  /// Kernel-flux line primitive over a triangle edge.
  auto flux_line_primitive() const -> OptimizedExpr {
    const auto polynomial = sym::as_polynomial(flux_moment_(rho), rho);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (!c.is_zero()) result += c * k_line_expr(static_cast<int>(p));
    }
    return optimize_line_(result);
  }

  /// Antigradient-flux line primitive over a triangle edge.
  auto antigrad_flux_line_primitive() const -> OptimizedExpr {
    const auto moment = tail_moment(3);
    const auto polynomial = sym::as_polynomial(moment, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (c.is_zero()) continue;
      if (p == 0) {
        result += c * (k_line_expr(0) - B);
      } else {
        result += c *
                  (eta * k_line_expr(static_cast<int>(p - 1)) -
                   pow(eta, p) * k_line_expr(0)) /
                  (p - 1);
      }
    }
    return optimize_line_(result);
  }

  /// Kernel-flux sector primitive (angular part) over a triangle.
  auto flux_sector() const -> OptimizedExpr {
    return optimize_sector_(flux_moment_(cutoff_));
  }

  /// Antigradient-flux sector primitive (angular part) over a triangle.
  auto antigrad_flux_sector() const -> OptimizedExpr {
    const auto moment = tail_moment(3);
    const auto polynomial = sym::as_polynomial(moment, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (c.is_zero()) continue;
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
    const auto polynomial = sym::as_polynomial(weight_, q);
    auto result = expr_context.integer(0);
    for (const auto& [p, c] :
         std::views::enumerate(polynomial.coefficients())) {
      if (c.is_zero()) continue;
      const auto e = p + 2;
      result += c * (pow(upper, e) - pow(eta, e)) / e;
    }
    return result;
  }

  // Optimize a radial primitive: reduce `rho`, Horner in `z` and `eta`, then
  // do common-subexpression elimination.
  static auto optimize_radial_(const Expr& e) -> OptimizedExpr {
    const auto rho_reduced = power_reduce(e, rho, pow(z, 2) + pow(eta, 2));
    return optimize_best({
        e,
        rho_reduced,
        sym::horner(rho_reduced, {z, eta}),
        sym::horner(rho_reduced, {eta, z}),
    });
  }

  // Optimize an edge primitive: reduce `rho` and `beta`, Horner, then
  // do common-subexpression elimination.
  static auto optimize_line_(const Expr& e) -> OptimizedExpr {
    const auto rho_reduced =
        power_reduce(e, rho, pow(z, 2) + pow(eta, 2) + pow(delta, 2));
    const auto beta_reduced =
        power_reduce(rho_reduced, beta, pow(eta, 2) + pow(delta, 2));
    std::vector<Expr> candidates{beta_reduced};
    for (const auto& order : std::array{
             std::array{z, delta, eta},
             std::array{z, eta, delta},
             std::array{delta, z, eta},
             std::array{delta, eta, z},
             std::array{eta, z, delta},
             std::array{eta, delta, z},
         }) {
      candidates.emplace_back(sym::horner(beta_reduced, order));
    }
    return optimize_best(candidates);
  }

  // Optimize a sector primitive: Horner in `eta`, then CSE.
  static auto optimize_sector_(const Expr& e) -> OptimizedExpr {
    return optimize_best({e, sym::horner(e, {eta})});
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
                              return segment.cutoff().as_rational();
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
           {2, expr_context.rational(1, 4) * pow(2 - q, 3)},
           {1, -pow(1 - q, 3)},
       }},
      {"QuarticSplineKernel",
       {
           {expr_context.rational(5, 2),
            pow(expr_context.rational(5, 2) - q, 4)},
           {expr_context.rational(3, 2),
            -5 * pow(expr_context.rational(3, 2) - q, 4)},
           {expr_context.rational(1, 2),
            10 * pow(expr_context.rational(1, 2) - q, 4)},
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
           {2,
            (1 + 3 * q + expr_context.rational(35, 12) * pow(q, 2)) *
                pow(1 - q / 2, 6)},
       }},
      {"EighthOrderWendlandKernel",
       {
           {2,
            (1 + 4 * q + expr_context.rational(25, 4) * pow(q, 2) +
             4 * pow(q, 3)) *
                pow(1 - q / 2, 8)},
       }},
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// C++ expression printer
//

/// Render symbolic expression as C++ code.
auto to_cxx(const Expr& expression) -> std::string { // NOLINT(*-no-recursion)
  if (expression.is_rational()) {
    const auto value = expression.as_rational();
    if (value.is_integer()) {
      return std::format("Num{{{}}}", value.numerator());
    }
    return std::format("Num{{{}.0 / {}.0}}",
                       value.numerator(),
                       value.denominator());
  }
  if (expression.is_symbol()) return std::string{expression.symbol_name()};
  if (expression.kind() == sym::ExprKind::add) {
    std::vector<std::string> parts;
    parts.reserve(expression.arity());
    for (std::size_t i = 0; i < expression.arity(); ++i) {
      parts.emplace_back(to_cxx(expression[i]));
    }
    return std::format("({})", str_join(parts, " + "));
  }
  if (expression.kind() == sym::ExprKind::mul) {
    sym::Rational coefficient{1};
    std::vector<std::string> parts;
    for (std::size_t i = 0; i < expression.arity(); ++i) {
      const auto arg = expression[i];
      if (arg.is_rational()) coefficient *= arg.as_rational();
      else parts.emplace_back(to_cxx(arg));
    }
    if (coefficient == sym::Rational{-1}) {
      if (parts.empty()) return "Num{-1.0}";
      parts.front() = "-" + parts.front();
    } else if (coefficient != sym::Rational{1}) {
      parts.insert(parts.begin(), to_cxx(expr_context.rational(coefficient)));
    }
    if (parts.empty()) return "Num{1.0}";
    return str_join(parts, " * ");
  }
  if (expression.kind() == sym::ExprKind::pow) {
    const auto base = expression[0];
    const auto exponent = expression[1];
    if (exponent.is_integer()) {
      const auto n = exponent.as_integer();
      if (n == -1) return std::format("inverse({})", to_cxx(base));
      if (n < -1) {
        const auto positive = static_cast<std::uint64_t>(-(n + 1)) + 1;
        return std::format("inverse(pow({}, {}))", to_cxx(base), positive);
      }
      return std::format("pow({}, {})", to_cxx(base), n);
    }
    return std::format("pow({}, {})", to_cxx(base), to_cxx(exponent));
  }
  TIT_THROW("Cannot translate expression to C++.");
  std::unreachable();
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
                 const OptimizedExpr& opt) {
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
  for (const auto& binding : opt.bindings()) {
    std::println(os,
                 "  const auto {} = {};",
                 to_cxx(binding.symbol),
                 to_cxx(binding.value));
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
