/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd/traits.hpp"

namespace tit::simd {

// NOLINTBEGIN(*-unused-parameter*)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD register mask type.
template<class Num, size_t Size>
class RegMask final {
public:

  /// Fill-initialize the register mask with zeroes.
  RegMask() noexcept {
    static_assert(false);
  }

}; // class RegMask

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD mask reduction functions
//

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD register type.
template<class Num, size_t Size>
class Reg final {
public:

  /// Associated SIMD mask type.
  using RegMask = tit::simd::RegMask<Num, Size>;

  /// Fill-initialize the register with zeroes.
  Reg() noexcept {
    static_assert(false);
  }

  /// Fill-initialize the register with the value @p q.
  explicit Reg(Num q) noexcept {
    static_assert(false);
  }

}; // class Reg

template<class Num, size_t Size>
concept available = !std::is_empty_v<Reg<Num, Size>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD arithmetic operations
//

/// SIMD unary plus operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator+(Reg<Num, Size> const& a) noexcept //
    -> Reg<Num, Size> const& {
  return a;
}

/// SIMD addition operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator+(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD addition with assignment operation.
template<class Num, size_t Size>
[[gnu::always_inline]]
inline auto operator+=(Reg<Num, Size>& a,
                       Reg<Num, Size> const& b) noexcept -> Reg<Num, Size>& {
  return a = a + b;
}

/// SIMD negation operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator-(Reg<Num, Size> const& a) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD subtraction operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator-(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD subtraction with assignment operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator-=(Reg<Num, Size>& a,
                       Reg<Num, Size> const& b) noexcept -> Reg<Num, Size>& {
  return a = a - b;
}

/// SIMD multiplication operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator*(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD multiplication with assignment operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator*=(Reg<Num, Size>& a,
                       Reg<Num, Size> const& b) noexcept -> Reg<Num, Size>& {
  return a = a * b;
}

/// SIMD division operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator/(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD division with assignment operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator/=(Reg<Num, Size>& a,
                       Reg<Num, Size> const& b) noexcept -> Reg<Num, Size>& {
  return a = a / b;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD comparison operations
//

/// SIMD "equal to" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator==(Reg<Num, Size> const& a,
                       Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  static_assert(false);
}

/// SIMD "not equal to" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator!=(Reg<Num, Size> const& a,
                       Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  return !(a == b);
}

/// SIMD "less than" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator<(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  static_assert(false);
}

/// SIMD "less than or equal to" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto operator<=(Reg<Num, Size> const& a,
                       Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  static_assert(false);
}

/// SIMD "greater than" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator>(Reg<Num, Size> const& a,
                      Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  return b < a;
}

/// SIMD "greater than or equal to" comparison operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto operator>=(Reg<Num, Size> const& a,
                       Reg<Num, Size> const& b) noexcept -> RegMask<Num, Size> {
  return b <= a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD algorithms
//

/// SIMD element-wise minimum algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto min(Reg<Num, Size> const& a,
                Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD element-wise maximum algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto max(Reg<Num, Size> const& a,
                Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD filter algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto filter(RegMask<Num, Size> const& m,
                   Reg<Num, Size> const& a) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD blend algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto blend(RegMask<Num, Size> const& m, //
                  Reg<Num, Size> const& a,
                  Reg<Num, Size> const& b) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD mathemathical functions
//

/// SIMD `floor` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto floor(Reg<Num, Size> const& a) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD `round` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto round(Reg<Num, Size> const& a) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD `ceil` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
inline auto ceil(Reg<Num, Size> const& a) noexcept -> Reg<Num, Size> {
  static_assert(false);
}

/// SIMD fused multiply-add operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto fma(Reg<Num, Size> const& a, //
                Reg<Num, Size> const& b,
                Reg<Num, Size> const& c) noexcept -> Reg<Num, Size> {
  return c + a * b;
}

/// SIMD fused multiply-subtract operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto fnma(Reg<Num, Size> const& a, //
                 Reg<Num, Size> const& b,
                 Reg<Num, Size> const& c) noexcept -> Reg<Num, Size> {
  return c - a * b;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SIMD reductions
//

/// SIMD horizontal sum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto sum(Reg<Num, Size> const& a) noexcept -> Num {
  static_assert(splittable<Num, Size>);
  auto const [high, low] = split(a);
  return sum(high, low);
}

/// SIMD horizontal minimum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto min_value(Reg<Num, Size> const& a) noexcept -> Num {
  static_assert(splittable<Num, Size>);
  auto const [high, low] = split(a);
  return min_value(high, low);
}

/// SIMD horizontal maximum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto max_value(Reg<Num, Size> const& a) noexcept -> Num {
  static_assert(splittable<Num, Size>);
  auto const [high, low] = split(a);
  return max_value(high, low);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-unused-parameter*)

} // namespace tit::simd
