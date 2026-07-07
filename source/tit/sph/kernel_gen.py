#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
from functools import cache
from pathlib import Path
import re
import shutil
import subprocess

import sympy as sp
from sympy.polys.polyerrors import PolynomialError

q, s, z, eta, rho, delta, beta, A, B, L = sp.symbols(
    "q s z eta rho delta beta A B L",
)
OptimizedExpr = tuple[list[tuple[sp.Symbol, sp.Expr]], sp.Expr]
MAX_DECIMAL_DIGITS = 3
SPILL_OP_COUNT = 5
SPILL_CXX_SIZE = 35
FROZEN_RATIONALS: dict[sp.Symbol, sp.Rational] = {}
FROZEN_RATIONAL_SYMBOLS: dict[sp.Rational, sp.Symbol] = {}
SECTION_SEPARATOR = "// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"


@dataclass(frozen=True)
class SupportTerm:
    cutoff: sp.Expr
    expr: sp.Expr


@dataclass(frozen=True)
class KernelSpec:
    slug: str
    terms: tuple[SupportTerm, ...]

    @property
    def class_name(self) -> str:
        return "".join(part.title() for part in self.slug.split("_")) + "Kernel"


def rational(num: int, den: int = 1) -> sp.Rational:
    return sp.Rational(num, den)


def spline_terms(
    degree: int,
    cutoffs: Iterable[sp.Expr],
    weights: Iterable[sp.Expr],
) -> tuple[SupportTerm, ...]:
    return tuple(
        SupportTerm(cutoff=cutoff, expr=weight * (cutoff - q) ** degree)
        for cutoff, weight in zip(cutoffs, weights, strict=True)
    )


def kernel_specs() -> list[KernelSpec]:
    one = sp.Integer(1)
    two = sp.Integer(2)

    return [
        KernelSpec(
            slug="cubic_spline",
            terms=spline_terms(
                3,
                [two, one],
                [rational(1, 4), -one],
            ),
        ),
        KernelSpec(
            slug="quartic_spline",
            terms=spline_terms(
                4,
                [rational(5, 2), rational(3, 2), rational(1, 2)],
                [one, -5 * one, 10 * one],
            ),
        ),
        KernelSpec(
            slug="quintic_spline",
            terms=spline_terms(
                5,
                [3 * one, two, one],
                [one, -6 * one, 15 * one],
            ),
        ),
        KernelSpec(
            slug="quartic_wendland",
            terms=(
                SupportTerm(
                    cutoff=two,
                    expr=(one + two * q) * (one - q / two) ** 4,
                ),
            ),
        ),
        KernelSpec(
            slug="sixth_order_wendland",
            terms=(
                SupportTerm(
                    cutoff=two,
                    expr=(one + 3 * q + rational(35, 12) * q**2)
                    * (one - q / two) ** 6,
                ),
            ),
        ),
        KernelSpec(
            slug="eighth_order_wendland",
            terms=(
                SupportTerm(
                    cutoff=two,
                    expr=(one + 4 * q + rational(25, 4) * q**2 + 4 * q**3)
                    * (one - q / two) ** 8,
                ),
            ),
        ),
    ]


@cache
def j_expr(power: int) -> sp.Expr:
    if power == 0:
        return z
    if power == 1:
        return (z * rho + eta**2 * L) / 2
    return sp.simplify(
        z * rho**power / (power + 1)
        + rational(power, power + 1) * eta**2 * j_expr(power - 2),
    )


@cache
def j_line_expr(power: int) -> sp.Expr:
    if power == 0:
        return z
    if power == 1:
        return (z * rho + beta**2 * L) / 2
    return sp.simplify(
        z * rho**power / (power + 1)
        + rational(power, power + 1) * beta**2 * j_line_expr(power - 2),
    )


@cache
def k_line_expr(power: int) -> sp.Expr:
    if power == 0:
        return A
    if power == 1:
        return delta * L + eta * B
    return sp.simplify(
        delta * j_line_expr(power - 2) + eta**2 * k_line_expr(power - 2),
    )


def flux_primitive_expr(term: SupportTerm) -> sp.Expr:
    poly = sp.Poly(sp.expand(term.expr), q)
    return sum(
        coeff * j_expr(power)
        for (power,), coeff in sorted(poly.as_dict().items())
    )


def flux_moment_expr(term: SupportTerm, upper: sp.Expr) -> sp.Expr:
    poly = sp.Poly(sp.expand(term.expr), q)
    return sum(
        coeff * (upper ** (power + 2) - eta ** (power + 2)) / (power + 2)
        for (power,), coeff in sorted(poly.as_dict().items())
    )


def flux_line_primitive_expr(term: SupportTerm) -> sp.Expr:
    poly = sp.Poly(sp.expand(flux_moment_expr(term, rho)), rho)
    return sum(
        coeff * k_line_expr(power)
        for (power,), coeff in sorted(poly.as_dict().items())
    )


def flux_sector_expr(term: SupportTerm) -> sp.Expr:
    return sp.expand(flux_moment_expr(term, term.cutoff))


def antiderivative_moment_term(term: SupportTerm) -> sp.Expr:
    return sp.factor(
        sp.integrate(term.expr.subs(q, s) * s, (s, q, term.cutoff)),
    )


def antiderivative_moment_3d_term(term: SupportTerm) -> sp.Expr:
    return sp.factor(
        sp.integrate(term.expr.subs(q, s) * s**2, (s, q, term.cutoff)),
    )


def antigrad_flux_primitive_expr(term: SupportTerm) -> sp.Expr:
    poly = sp.Poly(sp.expand(antiderivative_moment_term(term)), q)
    expr = sp.Integer(0)
    for (power,), coeff in sorted(poly.as_dict().items()):
        if power == 0:
            expr += coeff * A
        elif power == 1:
            expr += coeff * eta * L
        else:
            expr += coeff * eta * j_expr(power - 2)
    return expr


def antigrad_triangle_moment_expr(term: SupportTerm, upper: sp.Expr) -> sp.Expr:
    poly = sp.Poly(sp.expand(antiderivative_moment_3d_term(term)), q)
    expr = sp.Integer(0)
    for (power,), coeff in sorted(poly.as_dict().items()):
        if power == 0:
            expr += coeff * (1 / eta - 1 / upper)
        elif power == 1:
            if coeff != 0:
                msg = "Linear tail-moment terms require logarithmic primitives."
                raise ValueError(msg)
        else:
            expr += coeff * (upper ** (power - 1) - eta ** (power - 1)) / (
                power - 1
            )
    return expr


def antigrad_triangle_line_primitive_expr(term: SupportTerm) -> sp.Expr:
    poly = sp.Poly(sp.expand(antiderivative_moment_3d_term(term)), q)
    expr = sp.Integer(0)
    for (power,), coeff in sorted(poly.as_dict().items()):
        if power == 0:
            expr += coeff * (k_line_expr(0) / eta - B / eta)
        elif power == 1:
            if coeff != 0:
                msg = "Linear tail-moment terms require logarithmic primitives."
                raise ValueError(msg)
        else:
            expr += coeff * (
                k_line_expr(power - 1) - eta ** (power - 1) * k_line_expr(0)
            ) / (power - 1)
    return expr


def antigrad_triangle_sector_expr(term: SupportTerm) -> sp.Expr:
    return sp.expand(antigrad_triangle_moment_expr(term, term.cutoff))


def antigrad_triangle_flux_line_primitive_expr(term: SupportTerm) -> sp.Expr:
    return sp.factor(sp.cancel(eta * antigrad_triangle_line_primitive_expr(term)))


def antigrad_triangle_flux_sector_expr(term: SupportTerm) -> sp.Expr:
    return sp.factor(sp.cancel(eta * antigrad_triangle_sector_expr(term)))


def frozen_rational_symbol(value: sp.Rational) -> sp.Symbol:
    if value not in FROZEN_RATIONAL_SYMBOLS:
        symbol = sp.Symbol(f"fr{len(FROZEN_RATIONAL_SYMBOLS)}")
        FROZEN_RATIONAL_SYMBOLS[value] = symbol
        FROZEN_RATIONALS[symbol] = value
    return FROZEN_RATIONAL_SYMBOLS[value]


def freeze_rationals(expr: sp.Expr) -> sp.Expr:
    replacements = {
        value: frozen_rational_symbol(value)
        for value in sorted(
            (value for value in expr.atoms(sp.Rational) if value.q != 1),
            key=lambda value: (int(value.p), int(value.q)),
        )
    }
    return expr.xreplace(replacements)


def frozen_rational_cxx(value: sp.Rational) -> str:
    if decimal := terminating_decimal(value):
        return decimal
    return f"({value.p}.0 / {value.q}.0)"


def typed_rational_cxx(value: sp.Rational) -> str:
    code = frozen_rational_cxx(value)
    if code.startswith("(") and code.endswith(")"):
        code = code[1:-1]
    return f"Num{{{code}}}"


def terminating_decimal(value: sp.Rational) -> str | None:
    denominator = int(value.q)
    twos = 0
    while denominator % 2 == 0:
        twos += 1
        denominator //= 2
    fives = 0
    while denominator % 5 == 0:
        fives += 1
        denominator //= 5
    if denominator != 1:
        return None

    scale_exp = max(twos, fives)
    scale = 10**scale_exp
    numerator = int(value.p) * 2 ** (scale_exp - twos) * 5 ** (scale_exp - fives)
    is_negative = numerator < 0
    numerator = abs(numerator)
    whole, fraction = divmod(numerator, scale)
    fraction_text = f"{fraction:0{scale_exp}d}".rstrip("0")
    if len(fraction_text) > MAX_DECIMAL_DIGITS:
        return None
    if not fraction_text:
        decimal = f"{whole}.0"
    else:
        decimal = f"{whole}.{fraction_text}"
    if is_negative:
        return f"(-{decimal})"
    return decimal


def cxx_expr(expr: sp.Expr) -> str:
    code = sp.ccode(expr)
    for symbol, value in sorted(
        FROZEN_RATIONALS.items(),
        key=lambda item: len(str(item[0])),
        reverse=True,
    ):
        code = re.sub(
            rf"\b{re.escape(str(symbol))}\b",
            frozen_rational_cxx(value),
            code,
        )
    return code


def typed_cxx_expr(expr: sp.Expr) -> str:
    if isinstance(expr, sp.Symbol) and expr in FROZEN_RATIONALS:
        return typed_rational_cxx(FROZEN_RATIONALS[expr])
    if expr.is_Integer:
        return f"Num{{{int(expr)}.0}}"
    if expr.is_Rational:
        return typed_rational_cxx(expr)
    if isinstance(expr, sp.Float):
        return f"Num{{{expr}}}"
    if expr.is_Symbol:
        return str(expr)
    if isinstance(expr, sp.Add):
        return "(" + " + ".join(typed_cxx_expr(arg) for arg in expr.args) + ")"
    if isinstance(expr, sp.Mul):
        coeff, factors = expr.as_coeff_mul()
        parts = []
        if coeff != 1:
            parts.append(typed_cxx_expr(coeff))
        parts.extend(typed_cxx_expr(factor) for factor in factors)
        return " * ".join(parts) or "Num{1.0}"
    if isinstance(expr, sp.Pow):
        base, exponent = expr.args
        if exponent.is_Integer:
            if exponent == -1:
                return f"Num{{1.0}} / {typed_cxx_expr(base)}"
            if exponent < -1:
                return f"Num{{1.0}} / pow({typed_cxx_expr(base)}, {-int(exponent)})"
            return f"pow({typed_cxx_expr(base)}, {int(exponent)})"
        return f"pow({typed_cxx_expr(base)}, {typed_cxx_expr(exponent)})"
    return cxx_expr(expr)


def rho_power_reduced_expr(expr: sp.Expr) -> sp.Expr:
    rho_squared = z**2 + eta**2
    return power_reduced_expr(expr, rho, rho_squared)


def line_power_reduced_expr(expr: sp.Expr) -> sp.Expr:
    expr = power_reduced_expr(expr, rho, z**2 + eta**2 + delta**2)
    return power_reduced_expr(expr, beta, eta**2 + delta**2)


def power_reduced_expr(expr: sp.Expr, symbol: sp.Symbol, squared: sp.Expr) -> sp.Expr:
    replacements: dict[sp.Expr, sp.Expr] = {}
    for power in expr.atoms(sp.Pow):
        if power.base != symbol or not power.exp.is_Integer:
            continue
        exponent = int(power.exp)
        if exponent < 2:
            continue
        reduced_power = squared ** (exponent // 2)
        if exponent % 2 != 0:
            reduced_power *= symbol
        replacements[power] = reduced_power
    return expr.xreplace(replacements)


def normalized_expr(expr: sp.Expr) -> sp.Expr:
    return sp.collect(sp.expand(expr), [A, L, rho])


def normalized_line_expr(expr: sp.Expr) -> sp.Expr:
    return sp.collect(sp.expand(expr), [A, B, L, rho, beta])


def split_eta_laurent_expr(expr: sp.Expr) -> tuple[sp.Expr, sp.Expr]:
    regular = sp.Integer(0)
    singular_numerator = sp.Integer(0)
    for term in sp.Add.make_args(sp.expand(expr)):
        exponent = term.as_powers_dict().get(eta, sp.Integer(0))
        if not exponent.is_Integer:
            msg = f"Non-polynomial eta term: {term}."
            raise PolynomialError(msg)
        exponent = int(exponent)
        if exponent < -1:
            msg = f"Unsupported eta Laurent term: {term}."
            raise PolynomialError(msg)
        if exponent == -1:
            singular_numerator += term * eta
        else:
            regular += term
    return sp.expand(regular), sp.expand(singular_numerator)


def cse_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    replacements, reduced_exprs = sp.cse(
        expr,
        symbols=sp.numbered_symbols("t"),
        optimizations="basic",
    )
    return replacements, reduced_exprs[0]


def base_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    return cse_optimized_expr(normalized_expr(expr))


def horner_optimized_expr(expr: sp.Expr, *symbols: sp.Symbol) -> OptimizedExpr:
    expr = sp.collect(sp.expand(expr), [A, L, rho])
    expr = sp.horner(expr, *symbols)
    replacements, reduced_exprs = sp.cse(
        expr,
        symbols=sp.numbered_symbols("t"),
        optimizations="basic",
    )
    return replacements, reduced_exprs[0]


def frozen_horner_optimized_expr(
    expr: sp.Expr,
    *symbols: sp.Symbol,
) -> OptimizedExpr:
    expr = sp.collect(sp.expand(expr), [A, L, rho])
    expr = sp.horner(expr, *symbols)
    expr = freeze_rationals(expr)
    replacements, reduced_exprs = sp.cse(
        expr,
        symbols=sp.numbered_symbols("t"),
        optimizations="basic",
    )
    return replacements, reduced_exprs[0]


def line_base_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    return cse_optimized_expr(normalized_line_expr(expr))


def line_horner_optimized_expr(expr: sp.Expr, *symbols: sp.Symbol) -> OptimizedExpr:
    expr = normalized_line_expr(expr)
    expr = sp.horner(expr, *symbols)
    replacements, reduced_exprs = sp.cse(
        expr,
        symbols=sp.numbered_symbols("t"),
        optimizations="basic",
    )
    return replacements, reduced_exprs[0]


def frozen_line_horner_optimized_expr(
    expr: sp.Expr,
    *symbols: sp.Symbol,
) -> OptimizedExpr:
    expr = normalized_line_expr(expr)
    expr = sp.horner(expr, *symbols)
    expr = freeze_rationals(expr)
    replacements, reduced_exprs = sp.cse(
        expr,
        symbols=sp.numbered_symbols("t"),
        optimizations="basic",
    )
    return replacements, reduced_exprs[0]


def line_laurent_horner_optimized_expr(
    expr: sp.Expr,
    *symbols: sp.Symbol,
) -> OptimizedExpr:
    expr = normalized_line_expr(expr)
    regular, singular_numerator = split_eta_laurent_expr(expr)
    regular = sp.horner(normalized_line_expr(regular), *symbols)
    if singular_numerator != 0:
        singular_numerator = sp.factor(singular_numerator)
        sp.Poly(singular_numerator, eta, delta, z)
        singular = sp.horner(singular_numerator, *symbols) / eta
        regular += singular
    return cse_optimized_expr(regular)


def frozen_line_laurent_horner_optimized_expr(
    expr: sp.Expr,
    *symbols: sp.Symbol,
) -> OptimizedExpr:
    expr = normalized_line_expr(expr)
    regular, singular_numerator = split_eta_laurent_expr(expr)
    regular = sp.horner(normalized_line_expr(regular), *symbols)
    if singular_numerator != 0:
        singular_numerator = sp.factor(singular_numerator)
        sp.Poly(singular_numerator, eta, delta, z)
        singular = sp.horner(singular_numerator, *symbols) / eta
        regular += singular
    return cse_optimized_expr(freeze_rationals(regular))


def sector_base_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    return cse_optimized_expr(sp.expand(expr))


def frozen_sector_base_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    return cse_optimized_expr(freeze_rationals(sp.expand(expr)))


def sector_horner_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    expr = sp.horner(sp.expand(expr), eta)
    return cse_optimized_expr(expr)


def frozen_sector_horner_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    expr = sp.horner(sp.expand(expr), eta)
    return cse_optimized_expr(freeze_rationals(expr))


def sector_laurent_horner_optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    numerator = sp.expand(expr * eta)
    sp.Poly(numerator, eta)
    expr = sp.horner(numerator, eta) / eta
    return cse_optimized_expr(freeze_rationals(expr))


def optimized_op_count(optimized: OptimizedExpr) -> int:
    replacements, result = optimized
    return int(
        sum(sp.count_ops(value, visual=False) for _, value in replacements)
        + sp.count_ops(result, visual=False),
    )


def optimized_cxx_size(optimized: OptimizedExpr) -> int:
    replacements, result = optimized
    return sum(len(cxx_expr(value)) for _, value in replacements) + len(
        cxx_expr(result),
    )


def optimized_cost(optimized: OptimizedExpr) -> tuple[int, int]:
    return optimized_op_count(optimized), optimized_cxx_size(optimized)


def optimized_expr(expr: sp.Expr) -> OptimizedExpr:
    rho_reduced = rho_power_reduced_expr(expr)
    candidates = [
        base_optimized_expr(expr),
        base_optimized_expr(rho_reduced),
        horner_optimized_expr(rho_reduced, z, eta),
        horner_optimized_expr(rho_reduced, eta, z),
        frozen_horner_optimized_expr(rho_reduced, z, eta),
        frozen_horner_optimized_expr(rho_reduced, eta, z),
    ]
    return min(candidates, key=optimized_cost)


def optimized_line_expr(expr: sp.Expr) -> OptimizedExpr:
    power_reduced = line_power_reduced_expr(expr)
    candidates = [
        line_base_optimized_expr(expr),
        line_base_optimized_expr(power_reduced),
    ]
    for symbols in ((z, delta, eta), (delta, z, eta), (eta, delta, z)):
        try:
            candidates.append(line_horner_optimized_expr(power_reduced, *symbols))
            candidates.append(frozen_line_horner_optimized_expr(power_reduced, *symbols))
        except PolynomialError:
            pass
        try:
            candidates.append(
                line_laurent_horner_optimized_expr(power_reduced, *symbols),
            )
            candidates.append(
                frozen_line_laurent_horner_optimized_expr(power_reduced, *symbols),
            )
        except PolynomialError:
            pass
    beta_free_candidates = [
        candidate
        for candidate in candidates
        if not uses(candidate[0], candidate[1], beta)
    ]
    if beta_free_candidates:
        return min(beta_free_candidates, key=optimized_cost)
    return min(candidates, key=optimized_cost)


def optimized_sector_expr(expr: sp.Expr) -> OptimizedExpr:
    candidates = [
        sector_base_optimized_expr(expr),
        frozen_sector_base_optimized_expr(expr),
    ]
    try:
        candidates.append(sector_horner_optimized_expr(expr))
        candidates.append(frozen_sector_horner_optimized_expr(expr))
    except PolynomialError:
        pass
    try:
        candidates.append(sector_laurent_horner_optimized_expr(expr))
    except PolynomialError:
        pass
    return min(candidates, key=optimized_cost)


def temp_index(symbol: sp.Symbol) -> int | None:
    name = str(symbol)
    if not name.startswith("t") or not name[1:].isdigit():
        return None
    return int(name[1:])


def next_temp_index(replacements: list[tuple[sp.Symbol, sp.Expr]]) -> int:
    indices = [
        index
        for symbol, _ in replacements
        if (index := temp_index(symbol)) is not None
    ]
    if not indices:
        return 0
    return max(indices) + 1


def should_spill_expr(expr: sp.Expr) -> bool:
    if expr.is_Atom or isinstance(expr, sp.Pow):
        return False
    return (
        sp.count_ops(expr, visual=False) >= SPILL_OP_COUNT
        and len(cxx_expr(expr)) >= SPILL_CXX_SIZE
    )


def spill_expr(
    expr: sp.Expr,
    temp_index_start: int,
) -> tuple[list[tuple[sp.Symbol, sp.Expr]], sp.Expr, int]:
    replacements: list[tuple[sp.Symbol, sp.Expr]] = []
    temp_index_current = temp_index_start

    def visit(subexpr: sp.Expr, *, is_root: bool) -> sp.Expr:
        nonlocal temp_index_current
        if subexpr.is_Atom:
            return subexpr

        child_replacements = {}
        for arg in subexpr.args:
            new_arg = visit(arg, is_root=False)
            if new_arg != arg:
                child_replacements[arg] = new_arg
        if child_replacements:
            subexpr = subexpr.xreplace(child_replacements)

        if not is_root and should_spill_expr(subexpr):
            symbol = sp.Symbol(f"t{temp_index_current}")
            temp_index_current += 1
            replacements.append((symbol, subexpr))
            return symbol
        return subexpr

    return replacements, visit(expr, is_root=True), temp_index_current


def spill_large_subexpressions(optimized: OptimizedExpr) -> OptimizedExpr:
    replacements, result = optimized
    temp_index_current = next_temp_index(replacements)
    spilled_replacements: list[tuple[sp.Symbol, sp.Expr]] = []

    for symbol, value in replacements:
        extra_replacements, value, temp_index_current = spill_expr(
            value,
            temp_index_current,
        )
        spilled_replacements.extend(extra_replacements)
        spilled_replacements.append((symbol, value))

    extra_replacements, result, _ = spill_expr(result, temp_index_current)
    spilled_replacements.extend(extra_replacements)
    return spilled_replacements, result


def uses(
    replacements: list[tuple[sp.Symbol, sp.Expr]],
    result: sp.Expr,
    symbol: sp.Symbol,
) -> bool:
    return result.has(symbol) or any(value.has(symbol) for _, value in replacements)


def primitive_function(name: str, expr: sp.Expr) -> list[str]:
    replacements, result = spill_large_subexpressions(optimized_expr(expr))
    used_symbols = {
        symbol for symbol in (eta, z, rho, A, L) if uses(replacements, result, symbol)
    }

    def param(symbol: sp.Symbol) -> str:
        if symbol in used_symbols:
            return f"Num {symbol}"
        return f"[[maybe_unused]] Num {symbol}"

    lines = [
        "template<class Num>",
        f"constexpr auto {name}({param(eta)},",
        f"                    {param(z)},",
        f"                    {param(rho)},",
        f"                    {param(A)},",
        f"                    {param(L)}) noexcept -> Num {{",
    ]

    for symbol, value in replacements:
        lines.append(f"  const auto {symbol} = {typed_cxx_expr(value)};")
    lines.append(f"  return {typed_cxx_expr(result)};")
    lines.append("}")
    return lines


def line_primitive_function(name: str, expr: sp.Expr) -> list[str]:
    replacements, result = spill_large_subexpressions(optimized_line_expr(expr))
    used_symbols = {
        symbol
        for symbol in (eta, delta, z, rho, A, B, L)
        if uses(replacements, result, symbol)
    }

    def param(symbol: sp.Symbol) -> str:
        if symbol in used_symbols:
            return f"Num {symbol}"
        return f"[[maybe_unused]] Num {symbol}"

    lines = [
        "template<class Num>",
        f"constexpr auto {name}({param(eta)},",
        f"                    {param(delta)},",
        f"                    {param(z)},",
        f"                    {param(rho)},",
        f"                    {param(A)},",
        f"                    {param(B)},",
        f"                    {param(L)}) noexcept -> Num {{",
    ]

    for symbol, value in replacements:
        lines.append(f"  const auto {symbol} = {typed_cxx_expr(value)};")
    lines.append(f"  return {typed_cxx_expr(result)};")
    lines.append("}")
    return lines


def sector_function(name: str, expr: sp.Expr) -> list[str]:
    replacements, result = spill_large_subexpressions(optimized_sector_expr(expr))
    lines = [
        "template<class Num>",
        f"constexpr auto {name}(Num eta) noexcept -> Num {{",
    ]
    for symbol, value in replacements:
        lines.append(f"  const auto {symbol} = {typed_cxx_expr(value)};")
    lines.append(f"  return {typed_cxx_expr(result)};")
    lines.append("}")
    return lines


def num_expr(expr: sp.Expr) -> str:
    return typed_cxx_expr(expr)


def term_value_expr(term: SupportTerm) -> sp.Expr:
    return term.expr


def term_deriv_expr(term: SupportTerm) -> sp.Expr:
    return sp.diff(term.expr, q)


def truncated_term_text(term: SupportTerm, expr: sp.Expr) -> str:
    return f"(q < {num_expr(term.cutoff)} ? {typed_cxx_expr(expr)} : Num{{0.0}})"


def sum_text(terms: Iterable[str]) -> str:
    return " + ".join(terms) or "Num{0.0}"


def unit_radius_expr(spec: KernelSpec) -> sp.Expr:
    return max((term.cutoff for term in spec.terms), key=sp.default_sort_key)


def radial_moment_expr(spec: KernelSpec, power: int) -> sp.Expr:
    return sp.factor(
        sum(
            sp.integrate(term.expr * q**power, (q, 0, term.cutoff))
            for term in spec.terms
        ),
    )


def term_tail_moment_expr(term: SupportTerm, dim: int) -> sp.Expr:
    return sp.factor(
        sp.integrate(term.expr.subs(q, s) * s ** (dim - 1), (s, q, term.cutoff)),
    )


def weight_coeff_expr(spec: KernelSpec, dim: int) -> sp.Expr:
    if dim == 1:
        sphere_area = sp.Integer(2)
    elif dim == 2:
        sphere_area = sp.Integer(2)
    elif dim == 3:
        sphere_area = sp.Integer(4)
    else:
        msg = f"Unsupported kernel dimension: {dim}."
        raise ValueError(msg)
    return sp.factor(1 / (sphere_area * radial_moment_expr(spec, dim - 1)))


def weight_return_text(spec: KernelSpec, dim: int) -> str:
    coeff = typed_cxx_expr(weight_coeff_expr(spec, dim))
    if dim == 1:
        return coeff
    if coeff == "Num{1.0}":
        return "Num{std::numbers::inv_pi}"
    return f"{coeff} * Num{{std::numbers::inv_pi}}"


def weight_definition(spec: KernelSpec) -> list[str]:
    class_name = spec.class_name
    return [
        "template<>",
        "template<class Num, std::size_t Dim>",
        f"consteval auto {class_name}::weight() noexcept -> Num {{",
        "  if constexpr (Dim == 1) {",
        f"    return {weight_return_text(spec, 1)};",
        "  } else if constexpr (Dim == 2) {",
        f"    return {weight_return_text(spec, 2)};",
        "  } else if constexpr (Dim == 3) {",
        f"    return {weight_return_text(spec, 3)};",
        "  } else {",
        "    static_assert(false);",
        "  }",
        "}",
    ]


def radius_definition(spec: KernelSpec) -> list[str]:
    class_name = spec.class_name
    return [
        "template<>",
        "template<class Num>",
        f"consteval auto {class_name}::unit_radius() noexcept -> Num {{",
        f"  return {num_expr(unit_radius_expr(spec))};",
        "}",
    ]


def value_definition(spec: KernelSpec) -> list[str]:
    class_name = spec.class_name
    terms = [
        truncated_term_text(term, term_value_expr(term))
        for term in spec.terms
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {class_name}::unit_value(Num q) noexcept -> Num {{",
        f"  return {sum_text(terms)};",
        "}",
    ]


def deriv_definition(spec: KernelSpec) -> list[str]:
    class_name = spec.class_name
    terms = [
        truncated_term_text(term, term_deriv_expr(term))
        for term in spec.terms
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {class_name}::unit_deriv(Num q) noexcept -> Num {{",
        f"  return {sum_text(terms)};",
        "}",
    ]


def antideriv_moment_definition(spec: KernelSpec) -> list[str]:
    class_name = spec.class_name
    lines = [
        "template<>",
        "template<std::size_t Dim, class Num>",
        f"constexpr auto {class_name}::unit_antideriv_moment(Num q) noexcept -> Num {{",
    ]
    for dim in (1, 2, 3):
        prefix = "  if constexpr" if dim == 1 else "  } else if constexpr"
        terms = [
            truncated_term_text(term, term_tail_moment_expr(term, dim))
            for term in spec.terms
        ]
        lines.extend(
            [
                f"{prefix} (Dim == {dim}) {{",
                f"    return {sum_text(terms)};",
            ],
        )
    lines.extend(["  } else {", "    static_assert(false);", "  }", "}"])
    return lines


def segment_flux_term_text(spec: KernelSpec, index: int, term: SupportTerm) -> str:
    return (
        f"unit_segment_integral({num_expr(term.cutoff)}, eta, z_min, z_max, "
        f"impl::{spec.slug}_gen::unit_flux_{index}<Num>)"
    )


def triangle_flux_term_text(spec: KernelSpec, index: int, term: SupportTerm) -> str:
    namespace = f"impl::{spec.slug}_gen"
    return (
        f"unit_triangle_integral({num_expr(term.cutoff)}, eta, a, b, c, "
        f"{namespace}::unit_flux_line_{index}<Num>, "
        f"{namespace}::unit_flux_sector_{index}<Num>)"
    )


def segment_antigrad_flux_term_text(
    spec: KernelSpec,
    index: int,
    term: SupportTerm,
) -> str:
    return (
        f"unit_segment_integral({num_expr(term.cutoff)}, eta, z_min, z_max, "
        f"impl::{spec.slug}_gen::unit_antigrad_flux_{index}<Num>)"
    )


def triangle_antigrad_flux_term_text(
    spec: KernelSpec,
    index: int,
    term: SupportTerm,
) -> str:
    namespace = f"impl::{spec.slug}_gen"
    return (
        f"unit_triangle_integral({num_expr(term.cutoff)}, eta, a, b, c, "
        f"{namespace}::unit_antigrad_flux_line_{index}<Num>, "
        f"{namespace}::unit_antigrad_flux_sector_{index}<Num>)"
    )


def segment_flux_definition(spec: KernelSpec) -> list[str]:
    terms = [
        segment_flux_term_text(spec, index, term)
        for index, term in enumerate(spec.terms)
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {spec.class_name}::unit_flux(",
        "    Num eta,",
        "    Num z_min,",
        "    Num z_max) noexcept -> Num {",
        f"  return {sum_text(terms)};",
        "}",
    ]


def segment_antigrad_flux_definition(spec: KernelSpec) -> list[str]:
    terms = [
        segment_antigrad_flux_term_text(spec, index, term)
        for index, term in enumerate(spec.terms)
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {spec.class_name}::unit_antigrad_flux(",
        "    Num eta,",
        "    Num z_min,",
        "    Num z_max) noexcept -> Num {",
        f"  return {sum_text(terms)};",
        "}",
    ]


def triangle_flux_definition(spec: KernelSpec) -> list[str]:
    terms = [
        triangle_flux_term_text(spec, index, term)
        for index, term in enumerate(spec.terms)
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {spec.class_name}::unit_flux(",
        "    Num eta,",
        "    const Vec<Num, 2>& a,",
        "    const Vec<Num, 2>& b,",
        "    const Vec<Num, 2>& c) noexcept -> Num {",
        f"  return {sum_text(terms)};",
        "}",
    ]


def triangle_antigrad_flux_definition(spec: KernelSpec) -> list[str]:
    terms = [
        triangle_antigrad_flux_term_text(spec, index, term)
        for index, term in enumerate(spec.terms)
    ]
    return [
        "template<>",
        "template<class Num>",
        f"constexpr auto {spec.class_name}::unit_antigrad_flux(",
        "    Num eta,",
        "    const Vec<Num, 2>& a,",
        "    const Vec<Num, 2>& b,",
        "    const Vec<Num, 2>& c) noexcept -> Num {",
        f"  return {sum_text(terms)};",
        "}",
    ]


def kernel_helper_namespace(spec: KernelSpec) -> list[str]:
    lines = [
        f"namespace impl::{spec.slug}_gen {{",
        "",
    ]
    for index, term in enumerate(spec.terms):
        lines.extend([SECTION_SEPARATOR, ""])
        lines.extend(primitive_function(f"unit_flux_{index}", flux_primitive_expr(term)))
        lines.extend(["", SECTION_SEPARATOR, ""])
        lines.extend(
            primitive_function(
                f"unit_antigrad_flux_{index}",
                antigrad_flux_primitive_expr(term),
            ),
        )
        lines.extend(["", SECTION_SEPARATOR, ""])
        lines.extend(
            line_primitive_function(
                f"unit_flux_line_{index}",
                flux_line_primitive_expr(term),
            ),
        )
        lines.extend(["", SECTION_SEPARATOR, ""])
        lines.extend(
            line_primitive_function(
                f"unit_antigrad_flux_line_{index}",
                antigrad_triangle_flux_line_primitive_expr(term),
            ),
        )
        lines.extend(["", SECTION_SEPARATOR, ""])
        lines.extend(sector_function(f"unit_flux_sector_{index}", flux_sector_expr(term)))
        lines.extend(["", SECTION_SEPARATOR, ""])
        lines.extend(
            sector_function(
                f"unit_antigrad_flux_sector_{index}",
                antigrad_triangle_flux_sector_expr(term),
            ),
        )
        lines.append("")
    lines.extend([SECTION_SEPARATOR, "", f"}} // namespace impl::{spec.slug}_gen", ""])
    return lines


def kernel_method_definitions(spec: KernelSpec) -> list[str]:
    lines: list[str] = []
    definitions = (
        # weight_definition(spec),
        radius_definition(spec),
        value_definition(spec),
        deriv_definition(spec),
        antideriv_moment_definition(spec),
        segment_flux_definition(spec),
        triangle_flux_definition(spec),
        segment_antigrad_flux_definition(spec),
        triangle_antigrad_flux_definition(spec),
    )
    for definition in definitions:
        if not definition:
            continue
        if lines:
            lines.extend([SECTION_SEPARATOR, ""])
        lines.extend(definition)
        lines.append("")
    return lines


def kernel_section(spec: KernelSpec) -> list[str]:
    lines = [
        SECTION_SEPARATOR,
        "//",
        f"// {spec.class_name}",
        "//",
        "",
    ]
    lines.extend(kernel_helper_namespace(spec))
    lines.extend(kernel_method_definitions(spec))
    return lines


def generated_text() -> str:
    lines = [
        "/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\\",
        " * Part of BlueTit Solver, under the MIT License.",
        " * See /LICENSE.md for license information. SPDX-License-Identifier: MIT",
        "\\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */",
        "",
        "// This file was generated by source/tit/sph/kernel_gen.py.",
        "// Do not edit manually.",
        "",
        "#pragma once",
        "",
        "#include <cstddef>",
        # "#include <numbers>",
        "",
        '#include "tit/core/math.hpp"',
        '#include "tit/core/vec.hpp"',
        '#include "tit/sph/kernel.hpp" // NOLINT(misc-header-include-cycle)',
        "",
        "namespace tit::sph {",
        "",
    ]

    for spec in kernel_specs():
        lines.extend(kernel_section(spec))

    lines.append(SECTION_SEPARATOR)
    lines.append("")
    lines.append("} // namespace tit::sph")
    lines.extend(
        [
            "",
            "#ifdef TIT_SPH_KERNEL_GEN_UNDEF_DECLARATIONS_INCLUDED",
            "#undef TIT_SPH_KERNEL_DECLARATIONS_INCLUDED",
            "#undef TIT_SPH_KERNEL_GEN_UNDEF_DECLARATIONS_INCLUDED",
            "#endif",
        ],
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    output_path = Path(__file__).with_name("kernel.inl.hpp")
    output_path.write_text(generated_text())
    if formatter := shutil.which("clang-format-22") or shutil.which("clang-format"):
        subprocess.run([formatter, "-i", str(output_path)], check=True)
    print(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
