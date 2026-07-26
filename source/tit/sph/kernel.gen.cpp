/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/math.hpp"
#include "tit/core/poly.hpp"
#include "tit/core/rational.hpp"
#include "tit/core/str.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Symbolic math
//

/// Expression type.
///
/// Every expression the generator builds is a polynomial with rational
/// coefficients over the variables below. The square roots `rho` and `beta`
/// and the transcendental primitives `A`, `B` and `L` are carried as opaque
/// variables: the runtime code evaluates them and substitutes them into the
/// generated helpers.
using Expr = Poly;

const Expr q = Expr::var("q");         ///< Normalized radius.
const Expr z = Expr::var("z");         ///< Along-edge coordinate.
const Expr eta = Expr::var("eta");     ///< Wall distance.
const Expr rho = Expr::var("rho");     ///< Radius, `rho^2 = z^2 + eta^2`.
const Expr delta = Expr::var("delta"); ///< Edge offset.
const Expr beta = Expr::var("beta");   ///< `beta^2 = eta^2 + delta^2`.
const Expr A = Expr::var("A");         ///< `atan2` primitive.
const Expr L = Expr::var("L");         ///< `log1p` primitive.

/// Non-zero coefficients of the expression in the given variable, by power.
auto coeffs_in(const Expr& expr, std::string_view var) -> std::map<int, Expr> {
  std::map<int, Expr> result;
  for (std::size_t power = 0; power <= expr.degree(var); ++power) {
    if (auto coeff = expr.coeff(var, power); !coeff.is_zero()) {
      result.emplace(static_cast<int>(power), std::move(coeff));
    }
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Kernel math
//

/// `J_p`: primitive of `rho^p` along a segment, with `rho^2 = z^2 + eta^2`.
auto j_expr(int power) -> Expr {
  if (power == 0) return z;
  if (power == 1) return (z * rho + pow(eta, 2) * L) / 2;
  return (z * pow(rho, power) + power * pow(eta, 2) * j_expr(power - 2)) /
         Rational{power + 1};
}

/// `J_p` for the triangle edge, with `rho^2 = z^2 + beta^2`.
auto j_line_expr(int power) -> Expr {
  if (power == 0) return z;
  if (power == 1) return (z * rho + pow(beta, 2) * L) / 2;
  return (z * pow(rho, power) + power * pow(beta, 2) * j_line_expr(power - 2)) /
         Rational{power + 1};
}

/// `K_p`: edge primitive built on top of the line integrals `J`.
auto k_line_expr(int power) -> Expr {
  if (power == 0) return A;
  if (power == 1) return delta * L;
  return delta * j_line_expr(power - 2) + pow(eta, 2) * k_line_expr(power - 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A single piece of a piecewise-polynomial kernel support: the weight
/// polynomial `w(q)` valid for `q < cutoff`.
class Segment final {
public:

  /// Construct a support piece from its cutoff and weight polynomial.
  Segment(Rational cutoff, Expr weight)
      : cutoff_{cutoff}, weight_{std::move(weight)} {}

  /// Support cutoff of this piece.
  auto cutoff() const -> Rational {
    return cutoff_;
  }

  /// Weight polynomial `w(q)`.
  auto value() const -> Expr {
    return weight_;
  }

  /// Radial derivative `w'(q)`.
  auto deriv() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(weight_, "q")) {
      if (power >= 1) result += coeff * power * pow(q, power - 1);
    }
    return result;
  }

  /// Tail moment of order `dim`:
  /// integral of `q^(dim-1) w(q)` from `q` to cutoff.
  auto tail_moment(int dim) const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(weight_, "q")) {
      result += coeff * pow(q, power) / Rational{dim + power};
    }
    const auto at_cutoff = result.substitute("q", cutoff_).as_rational();
    return pow(cutoff_, dim) * at_cutoff - pow(q, dim) * result;
  }

  /// Kernel-flux primitive over a clipped 2D segment.
  auto flux_primitive() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(weight_, "q")) {
      result += coeff * j_expr(power);
    }
    return reduce_radial_(result);
  }

  /// Antigradient-flux primitive over a clipped 2D segment.
  auto antigrad_flux_primitive() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(tail_moment(2), "q")) {
      if (power == 0) result += coeff * A;
      else if (power == 1) result += coeff * eta * L;
      else result += coeff * eta * j_expr(power - 2);
    }
    return reduce_radial_(result);
  }

  /// Kernel-flux line primitive over a triangle edge.
  auto flux_line_primitive() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(flux_moment_(rho), "rho")) {
      result += coeff * k_line_expr(power);
    }
    return reduce_line_(result);
  }

  /// Antigradient-flux line primitive over a triangle edge.
  auto antigrad_flux_line_primitive() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(tail_moment(3), "q")) {
      if (power == 0) {
        result += coeff * k_line_expr(0);
      } else {
        result +=
            coeff *
            (eta * k_line_expr(power - 1) - pow(eta, power) * k_line_expr(0)) /
            Rational{power - 1};
      }
    }
    return reduce_line_(result);
  }

  /// Kernel-flux sector primitive (angular part) over a triangle.
  auto flux_sector() const -> Expr {
    return flux_moment_(cutoff_);
  }

  /// Antigradient-flux sector primitive (angular part) over a triangle.
  auto antigrad_flux_sector() const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(tail_moment(3), "q")) {
      if (power == 0) {
        result += coeff * (1 - eta / cutoff_);
      } else {
        result += coeff * (eta * pow(cutoff_, power - 1) - pow(eta, power)) /
                  Rational{power - 1};
      }
    }
    return result;
  }

private:

  // Moment of `w(q)` with `upper^(p+2)` bounds (2D flux, `rho`-parameterized).
  auto flux_moment_(const Expr& upper) const -> Expr {
    Expr result;
    for (const auto& [power, coeff] : coeffs_in(weight_, "q")) {
      const auto exponent = power + 2;
      result += coeff * (pow(upper, exponent) - pow(eta, exponent)) /
                Rational{exponent};
    }
    return result;
  }

  // Rewrite the powers of `rho` in a radial primitive, so that the generated
  // code can reuse the precomputed radius.
  static auto reduce_radial_(const Expr& expr) -> Expr {
    return expr.reduce_square("rho", pow(z, 2) + pow(eta, 2));
  }

  // Rewrite the powers of `rho` and `beta` in an edge primitive.
  static auto reduce_line_(const Expr& expr) -> Expr {
    return expr.reduce_square("rho", pow(z, 2) + pow(eta, 2) + pow(delta, 2))
        .reduce_square("beta", pow(eta, 2) + pow(delta, 2));
  }

  Rational cutoff_;
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
  auto unit_radius() const -> Rational {
    return std::ranges::max(segments_, {}, &Segment::cutoff).cutoff();
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
           {2, Rational{1, 4} * pow(2 - q, 3)},
           {1, -pow(1 - q, 3)},
       }},
      {"QuarticSplineKernel",
       {
           {Rational{5, 2}, pow(Rational{5, 2} - q, 4)},
           {Rational{3, 2}, -5 * pow(Rational{3, 2} - q, 4)},
           {Rational{1, 2}, 10 * pow(Rational{1, 2} - q, 4)},
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
           {2, (1 + 3 * q + Rational{35, 12} * pow(q, 2)) * pow(1 - q / 2, 6)},
       }},
      {"EighthOrderWendlandKernel",
       {
           {2,
            (1 + 4 * q + Rational{25, 4} * pow(q, 2) + 4 * pow(q, 3)) *
                pow(1 - q / 2, 8)},
       }},
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// C++ expression printer
//

/// Rendered C++ expression.
class Code final {
public:

  /// Render a plain number.
  static auto number(const Rational& value) -> Code {
    auto text = value.is_integer() ?
                    std::format("Num{{{}}}", value.num()) :
                    std::format("Num{{{}.0 / {}.0}}", value.num(), value.den());
    return Code{std::move(text), false, value};
  }

  /// Render a sum of two or more terms.
  static auto sum(std::string text) -> Code {
    return Code{std::move(text), true, std::nullopt};
  }

  /// Render a term that is not a sum and not a plain number.
  static auto atom(std::string text) -> Code {
    return Code{std::move(text), false, std::nullopt};
  }

  /// Expression text.
  auto text() const -> const std::string& {
    return text_;
  }

  /// Expression text as an operand of a product, parenthesized if needed.
  auto factor() const -> std::string {
    return is_sum_ ? std::format("({})", text_) : text_;
  }

  /// Value of this expression, if it is a plain number.
  auto value() const -> const std::optional<Rational>& {
    return value_;
  }

private:

  Code(std::string text, bool is_sum, std::optional<Rational> value)
      : text_{std::move(text)}, is_sum_{is_sum}, value_{value} {}

  std::string text_;
  bool is_sum_;
  std::optional<Rational> value_;

}; // class Code

/// Render a rational number as C++ code.
auto to_cxx(const Rational& value) -> std::string {
  return Code::number(value).text();
}

/// Render a monomial as C++ code.
auto render_monomial(const Monomial& mono) -> std::string {
  std::vector<std::string> factors;
  for (const auto& [name, power] : mono.powers()) {
    factors.emplace_back(power == 1 ? name :
                                      std::format("pow({}, {})", name, power));
  }
  return str_join(factors, " * ");
}

/// Render an expression as a plain sum of its terms.
auto render_flat(const Expr& expr) -> Code {
  if (expr.is_constant()) return Code::number(expr.as_rational());
  std::vector<std::string> terms;
  for (const auto& [mono, coeff] : expr.terms()) {
    if (mono.is_unit()) {
      terms.emplace_back(to_cxx(coeff));
    } else if (coeff == 1) {
      terms.emplace_back(render_monomial(mono));
    } else if (coeff == -1) {
      terms.emplace_back(std::format("-{}", render_monomial(mono)));
    } else {
      terms.emplace_back(
          std::format("{} * {}", to_cxx(coeff), render_monomial(mono)));
    }
  }
  if (terms.size() > 1) return Code::sum(str_join(terms, " + "));
  return Code::atom(std::move(terms.front()));
}

/// Multiply a power of a variable by an already rendered expression.
auto render_product(std::string_view var_text,
                    std::size_t power,
                    const Code& code) -> std::string {
  auto power_text = power == 1 ? std::string{var_text} :
                                 std::format("pow({}, {})", var_text, power);
  if (code.value() == Rational{1}) return power_text;
  if (code.value() == Rational{-1}) return std::format("-{}", power_text);
  // A plain number reads better as the leading operand.
  if (code.value().has_value()) {
    return std::format("{} * {}", code.text(), power_text);
  }
  return std::format("{} * {}", power_text, code.factor());
}

/// Render an expression in the multivariate Horner form about the origin,
/// nesting the variables in the given order.
///
/// Runs of zero coefficients are folded into a single power, so that a factor
/// common to the remaining terms is taken out rather than repeated.
auto render_horner(const Expr& expr, std::span<const std::string_view> vars)
    -> Code {
  if (vars.empty()) return render_flat(expr);

  // Pull out whatever divides every term. The transcendental primitives are
  // never nested, so without this a factor common to a whole group -- `L` in
  // `L * a + eta^2 * (L * b + ...)` -- would be repeated in each term.
  if (const auto factor = expr.common_factor(); !factor.is_unit()) {
    const auto rest = render_horner(expr.divide(factor), vars);
    return Code::atom(
        std::format("{} * {}", render_monomial(factor), rest.factor()));
  }

  const auto var = vars.front();
  const auto& var_text = var;
  const auto rest = vars.subspan(1);

  const auto degree = expr.degree(var);
  auto result = render_horner(expr.coeff(var, degree), rest);
  std::size_t pending = 0;
  for (auto power = degree; power-- > 0;) {
    ++pending;
    const auto coeff = expr.coeff(var, power);
    if (coeff.is_zero()) continue;
    const auto tail = render_product(var_text, pending, result);
    const auto head = render_horner(coeff, rest);
    result = Code::sum(std::format("{} + {}", head.text(), tail));
    pending = 0;
  }
  if (pending > 0) {
    result = Code::atom(render_product(var_text, pending, result));
  }
  return result;
}

/// Render an expression in the multivariate Horner form about the origin.
auto render_horner(const Expr& expr,
                   std::initializer_list<std::string_view> vars) -> Code {
  return render_horner(expr, std::span{vars});
}

/// Render a univariate expression with its root at `root` factored out, as
/// `(var - root)^m * P(var)`, where `P` no longer vanishes at `root`.
///
/// Everything that is built by integrating the kernel weight inherits the
/// weight's root at the support cutoff, usually with an even higher
/// multiplicity: the weight and its radial derivative vanish there, and so do
/// the tail moments and the triangle sector primitives. Keeping that root as an
/// explicit factor makes the generated code exact at the cutoff and much more
/// compact, while the remaining factor is expanded about the origin, where the
/// kernel is largest. Expanding about either point alone would suffer
/// cancellation at the other.
///
/// The radial and edge flux primitives are the exception: they are
/// antiderivatives evaluated at the clipped endpoints rather than quantities
/// that vanish at the cutoff, so they have no such root and are rendered as
/// plain Horner forms.
auto render_root_factored(const Expr& expr,
                          std::string_view var,
                          const Rational& root) -> Code {
  if (expr.is_zero()) return Code::number(Rational{});

  // Find the multiplicity of the root.
  const auto var_expr = Expr::var(var);
  const auto shifted = expr.substitute(var, var_expr + root);
  const auto degree = shifted.degree(var);
  std::size_t mult = 0;
  while (mult <= degree && shifted.coeff(var, mult).is_zero()) ++mult;
  if (mult == 0) return render_horner(expr, {var});

  // Divide the root out and return to the powers of the variable.
  Expr rest = shifted.coeff(var, mult);
  for (auto power = mult + 1; power <= degree; ++power) {
    rest += shifted.coeff(var, power) * pow(var_expr, power - mult);
  }
  rest = rest.substitute(var, var_expr - root);

  const auto root_text = std::format("({} + {})", to_cxx(-root), var);
  return Code::atom(
      render_product(root_text, mult, render_horner(rest, {var})));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A support term guarded by its cutoff: `(q < cutoff ? expr : Num{0})`.
auto truncated(const Segment& segment, const Code& code) -> std::string {
  return std::format("(q < {} ? {} : Num{{0}})",
                     to_cxx(segment.cutoff()),
                     code.text());
}

/// Join truncated support terms into a single returned sum expression.
auto join_terms(const std::vector<std::string>& terms) -> std::string {
  return terms.empty() ? "Num{0}" : str_join(terms, " + ");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Generated helper functions
//

/// Fully-qualified name of a per-segment helper object.
auto helper_ref(const Kernel& kernel,
                std::string_view name,
                std::ptrdiff_t index) -> std::string {
  return std::format("impl::{}_gen::{}_{}", kernel.name(), name, index);
}

/// Emit a helper from an expression, as a named generic lambda rather than a
/// plain function template.
///
/// The integrals instantiate these at both the scalar number type and a SIMD
/// register, so the caller has to be able to pick the type itself. A function
/// pointer would have to be bound to one instantiation up front.
void emit_helper(std::ostream& os,
                 const std::string& name,
                 std::initializer_list<std::string_view> params,
                 std::initializer_list<std::string_view> horner_vars,
                 const Expr& expr,
                 const std::optional<Rational>& root = std::nullopt) {
  std::println(os, "inline constexpr auto {} =", name);
  const std::string head = "    []<class Num>(";
  const std::string cont(head.size(), ' ');
  std::print(os, "{}", head);
  for (const auto& [i, param] : std::views::enumerate(params)) {
    if (i != 0) std::print(os, ",\n{}", cont);
    if (!expr.uses(param)) std::print(os, "[[maybe_unused]] ");
    std::print(os, "Num {}", param);
  }
  std::println(os, ") constexpr noexcept -> Num {{");
  if (root.has_value()) {
    TIT_ENSURE(horner_vars.size() == 1,
               "Root factoring needs a single variable!");
  }
  const auto code =
      root.has_value() ?
          render_root_factored(expr, *horner_vars.begin(), *root) :
          render_horner(expr, horner_vars);
  std::println(os, "      return {};", code.text());
  std::println(os, "    }};");
  std::println(os);
}

/// Emit the `impl::<name>_gen` namespace of per-segment helpers.
void emit_helpers(std::ostream& os, const Kernel& kernel) {
  std::println(os, "namespace impl::{}_gen {{", kernel.name());
  std::println(os);
  for (const auto& [i, segment] : std::views::enumerate(kernel.segments())) {
    emit_helper(os,
                std::format("unit_flux_{}", i),
                {"eta", "z", "rho", "A", "L"},
                {"z", "eta"},
                segment.flux_primitive());
    emit_helper(os,
                std::format("unit_antigrad_flux_{}", i),
                {"eta", "z", "rho", "A", "L"},
                {"z", "eta"},
                segment.antigrad_flux_primitive());
    emit_helper(os,
                std::format("unit_flux_line_{}", i),
                {"eta", "delta", "z", "rho", "A", "L"},
                {"z", "delta", "eta"},
                segment.flux_line_primitive());
    emit_helper(os,
                std::format("unit_antigrad_flux_line_{}", i),
                {"eta", "delta", "z", "rho", "A", "L"},
                {"z", "delta", "eta"},
                segment.antigrad_flux_line_primitive());
    emit_helper(os,
                std::format("unit_flux_sector_{}", i),
                {"eta"},
                {"eta"},
                segment.flux_sector(),
                segment.cutoff());
    emit_helper(os,
                std::format("unit_antigrad_flux_sector_{}", i),
                {"eta"},
                {"eta"},
                segment.antigrad_flux_sector(),
                segment.cutoff());
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
    terms.emplace_back(truncated(
        segment,
        render_root_factored(segment.value(), "q", segment.cutoff())));
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
    terms.emplace_back(truncated(
        segment,
        render_root_factored(segment.deriv(), "q", segment.cutoff())));
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
      terms.emplace_back(
          truncated(segment,
                    render_root_factored(segment.tail_moment(dim),
                                         "q",
                                         segment.cutoff())));
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
