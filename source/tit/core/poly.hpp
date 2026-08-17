/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/rational.hpp"
#include "tit/core/str.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Monomial.
//

/// Product of powers of the named variables.
///
/// Variables with zero power are never stored, so that equal monomials always
/// compare equal.
class Monomial final {
public:

  /// Powers of the variables, ordered by variable name.
  using Powers = StrMap<std::size_t>;

  /// Construct the unit monomial.
  Monomial() = default;

  /// Construct the monomial that is the given variable itself.
  static auto var(std::string_view name) -> Monomial {
    return Monomial{}.with_power(name, 1);
  }

  /// Powers of the variables present in this monomial.
  auto powers() const -> const Powers& {
    return powers_;
  }

  /// Power of the given variable.
  auto power(std::string_view name) const -> std::size_t {
    const auto iter = powers_.find(name);
    return iter != powers_.end() ? iter->second : 0;
  }

  /// Is this the unit monomial?
  auto is_unit() const -> bool {
    return powers_.empty();
  }

  /// Copy of this monomial with the power of the given variable replaced.
  auto with_power(std::string_view name, std::size_t power) const -> Monomial {
    TIT_ENSURE(!name.empty(), "Variable name must not be empty!");
    auto result = *this;
    if (power == 0) result.powers_.erase(std::string{name});
    else result.powers_.insert_or_assign(std::string{name}, power);
    return result;
  }

  /// Multiply two monomials.
  friend auto operator*(const Monomial& a, const Monomial& b) -> Monomial {
    auto result = a;
    for (const auto& [name, power] : b.powers_) {
      result.powers_[name] = result.power(name) + power;
    }
    return result;
  }

  /// Compare two monomials by their variable names and powers.
  /// @{
  friend auto operator==(const Monomial& a, const Monomial& b)
      -> bool = default;
  friend auto operator<=>(const Monomial& a, const Monomial& b)
      -> std::strong_ordering = default;
  /// @}

private:

  Powers powers_;

}; // class Monomial

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Polynomial.
//

/// Multivariate polynomial with exact rational coefficients over the named
/// variables.
///
/// The polynomial is always kept in the expanded normal form: terms with equal
/// monomials are merged, and terms with zero coefficients are not stored.
/// Integers and rationals implicitly convert to constant polynomials, so the
/// usual mixed arithmetic just works.
class Poly final {
public:

  /// Terms of the polynomial, ordered by monomial.
  using Terms = std::map<Monomial, Rational>;

  /// Construct the zero polynomial.
  Poly() = default;

  /// Construct a constant polynomial.
  /// @{
  explicit(false) Poly(const Rational& coeff) {
    add_term_(Monomial{}, coeff);
  }
  explicit(false) Poly(std::int64_t coeff) : Poly{Rational{coeff}} {}
  /// @}

  /// Construct the polynomial that is the given variable itself.
  static auto var(std::string_view name) -> Poly {
    return term(Monomial::var(name), Rational{1});
  }

  /// Terms of this polynomial, ordered by monomial.
  auto terms() const -> const Terms& {
    return terms_;
  }

  /// Is this polynomial zero?
  auto is_zero() const -> bool {
    return terms_.empty();
  }

  /// Is this polynomial a constant?
  auto is_constant() const -> bool {
    return terms_.empty() ||
           (terms_.size() == 1 && terms_.begin()->first.is_unit());
  }

  /// Value of this polynomial, assuming it is a constant.
  auto as_rational() const -> Rational {
    TIT_ENSURE(is_constant(), "Polynomial is not a constant!");
    return is_zero() ? Rational{} : terms_.begin()->second;
  }

  /// Degree of this polynomial in the given variable.
  auto degree(std::string_view name) const -> std::size_t {
    std::size_t result = 0;
    for (const auto& [mono, _] : terms_) {
      result = std::max(result, mono.power(name));
    }
    return result;
  }

  /// Does this polynomial depend on the given variable?
  auto uses(std::string_view name) const -> bool {
    return std::ranges::any_of(terms_, [name](const auto& term) {
      return term.first.power(name) != 0;
    });
  }

  /// Largest monomial that divides every term of this polynomial.
  auto common_factor() const -> Monomial {
    if (is_zero()) return Monomial{};
    auto result = terms_.begin()->first;
    for (const auto& [mono, _] : terms_) {
      Monomial next;
      for (const auto& [name, power] : result.powers()) {
        if (const auto common = std::min(power, mono.power(name)); common != 0) {
          next = next.with_power(name, common);
        }
      }
      result = next;
      if (result.is_unit()) break;
    }
    return result;
  }

  /// Divide every term by a monomial that divides this polynomial.
  auto divide(const Monomial& mono) const -> Poly {
    Poly result;
    for (const auto& [term, coeff] : terms_) {
      auto quotient = term;
      for (const auto& [name, power] : mono.powers()) {
        const auto term_power = term.power(name);
        TIT_ENSURE(term_power >= power, "Monomial is not a common factor!");
        quotient = quotient.with_power(name, term_power - power);
      }
      result.add_term_(quotient, coeff);
    }
    return result;
  }

  /// Coefficient of the given power of the given variable, itself a polynomial
  /// in the remaining variables.
  auto coeff(std::string_view name, std::size_t power) const -> Poly {
    Poly result;
    for (const auto& [mono, c] : terms_) {
      if (mono.power(name) == power) {
        result.add_term_(mono.with_power(name, 0), c);
      }
    }
    return result;
  }

  /// Substitute a polynomial for the given variable.
  auto substitute(std::string_view name, const Poly& value) const -> Poly {
    const auto powers = powers_of_(value, degree(name));
    Poly result;
    for (const auto& [mono, c] : terms_) {
      const auto power = mono.power(name);
      result += term(mono.with_power(name, 0), c) * powers[power];
    }
    return result;
  }

  /// Rewrite the powers of the given variable in terms of its square, leaving
  /// the variable itself in at most the first power.
  ///
  /// This is meant for the variables that stand for square roots, whose square
  /// is a polynomial in the other variables.
  auto reduce_square(std::string_view name, const Poly& var_sqr) const -> Poly {
    std::size_t max_half = 0;
    for (const auto& [mono, _] : terms_) {
      max_half = std::max(max_half, mono.power(name) / 2);
    }
    const auto powers = powers_of_(var_sqr, max_half);
    Poly result;
    for (const auto& [mono, c] : terms_) {
      const auto power = mono.power(name);
      result += term(mono.with_power(name, power % 2), c) * powers[power / 2];
    }
    return result;
  }

  /// Arithmetic operators.
  /// @{
  friend auto operator-(const Poly& a) -> Poly {
    Poly result;
    for (const auto& [mono, c] : a.terms_) result.add_term_(mono, -c);
    return result;
  }
  friend auto operator+(const Poly& a, const Poly& b) -> Poly {
    auto result = a;
    for (const auto& [mono, c] : b.terms_) result.add_term_(mono, c);
    return result;
  }
  friend auto operator-(const Poly& a, const Poly& b) -> Poly {
    auto result = a;
    for (const auto& [mono, c] : b.terms_) result.add_term_(mono, -c);
    return result;
  }
  friend auto operator*(const Poly& a, const Poly& b) -> Poly {
    Poly result;
    for (const auto& [a_mono, a_c] : a.terms_) {
      for (const auto& [b_mono, b_c] : b.terms_) {
        result.add_term_(a_mono * b_mono, a_c * b_c);
      }
    }
    return result;
  }
  friend auto operator/(const Poly& a, const Rational& b) -> Poly {
    TIT_ENSURE(!b.is_zero(), "Polynomial division by zero!");
    const auto b_inverse = Rational{1} / b;
    Poly result;
    for (const auto& [mono, c] : a.terms_) {
      result.add_term_(mono, c * b_inverse);
    }
    return result;
  }
  // Note: the compound operators below go through the binary ones, which copy
  // the left operand first. Merging into `a` in place would be faster, but it
  // would also read `b`'s terms while modifying them whenever the operands are
  // the same object.
  friend auto operator+=(Poly& a, const Poly& b) -> Poly& {
    a = a + b;
    return a;
  }
  /// @}

  /// Compare two polynomials.
  friend auto operator==(const Poly& a, const Poly& b) -> bool = default;

private:

  // Build a single-term polynomial.
  static auto term(const Monomial& mono, const Rational& coeff) -> Poly {
    Poly result;
    result.add_term_(mono, coeff);
    return result;
  }

  // Powers of the given polynomial, from zero up to and including `max_power`.
  static auto powers_of_(const Poly& base, std::size_t max_power)
      -> std::vector<Poly> {
    std::vector<Poly> result{Poly{Rational{1}}};
    while (result.size() <= max_power) result.push_back(result.back() * base);
    return result;
  }

  // Add a term, merging it with the existing one and dropping the zeros.
  void add_term_(const Monomial& mono, const Rational& coeff) {
    if (coeff.is_zero()) return;
    const auto [iter, inserted] = terms_.try_emplace(mono, coeff);
    if (inserted) return;
    iter->second += coeff;
    if (iter->second.is_zero()) terms_.erase(iter);
  }

  Terms terms_;

}; // class Poly

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
