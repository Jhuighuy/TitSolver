/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec/vec_mask.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector class.
//

/// Column vector.
template<class Num, size_t Dim>
class Vec final {
public:

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() : Vec(Num{0}) {}

  /// Fill-initialize the vector with the value @p q.
  constexpr explicit(Dim > 1) Vec(const Num& q) : col_{fill_array<Dim>(q)} {}

  /// Construct a vector with elements @p qs.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args &&> && ...)
  constexpr Vec(Args&&... qs) // NOSONAR
      : col_{make_array<Dim, Num>(std::forward<Args>(qs)...)} {}

  /// Vector dimensionality.
  constexpr auto dim() const -> ssize_t {
    return Dim;
  }

  /// Vector element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> const Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  /// @}

private:

  std::array<Num, Dim> col_;

}; // class Vec

/// Column vector with SIMD support.
template<simd::supported_type Num, size_t Dim>
class Vec<Num, Dim> final {
public:

  /// Type of the underlying register that is used.
  using Reg = simd::deduce_reg_t<Num, Dim>;

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::deduce_size_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::deduce_count_v<Num, Dim>;

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() noexcept {
    if consteval {
      col_ = fill_array<Dim>(Num{0});
    } else {
      regs_ = fill_array<RegCount>(Reg{});
    }
  }

  /// Fill-initialize the vector with the value @p q.
  constexpr explicit(Dim > 1) Vec(Num q) noexcept {
    if consteval {
      col_ = fill_array<Dim>(q);
    } else {
      regs_ = fill_array<RegCount>(Reg(q));
    }
  }

  /// Construct a vector with elements @p qs.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args &&> && ...)
  constexpr Vec(Args&&... qs) // NOSONAR
      : col_{make_array<Dim, Num>(std::forward<Args>(qs)...)} {}

  /// Move-construct the vector.
  constexpr Vec(Vec&& other) noexcept {
    if consteval {
      col_ = std::move(other.col_);
    } else {
      regs_ = std::move(other.regs_);
    }
  }

  /// Move-assign the vector.
  constexpr auto operator=(Vec&& other) noexcept -> Vec& {
    if consteval {
      col_ = std::move(other.col_);
    } else {
      regs_ = std::move(other.regs_);
    }
    return *this;
  }

  /// Copy-construct the vector.
  constexpr Vec(const Vec& other) {
    if consteval {
      col_ = other.col_;
    } else {
      regs_ = other.regs_;
    }
  }

  /// Copy-assign the vector.
  constexpr auto operator=(const Vec& other) -> Vec& { // NOLINT(cert-oop54-cpp)
    if consteval {
      col_ = other.col_;
    } else {
      regs_ = other.regs_;
    }
    return *this;
  }

  /// Destroy the vector.
  constexpr ~Vec() = default;

  /// Vector dimensionality.
  constexpr auto dim() const -> ssize_t {
    return Dim;
  }

  /// Element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> const Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  /// @}

  /// Underlying register at index.
  /// @{
  auto reg(size_t i) noexcept -> Reg& {
    TIT_ASSERT(i < RegCount, "Register index is out of range!");
    return regs_[i];
  }
  auto reg(size_t i) const noexcept -> const Reg& {
    TIT_ASSERT(i < RegCount, "Register index is out of range!");
    return regs_[i];
  }
  /// @}

  // NOLINTEND(*-type-union-access)

private:

  union {
    std::array<Num, Dim> col_{};
    std::array<Reg, RegCount> regs_;
  };

}; // class Vec<SIMD>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Casts
//

/// Create a unit vector.
template<size_t Axis = 0, class Num, size_t Dim>
constexpr auto unit(const Vec<Num, Dim>& /*a*/) -> Vec<Num, Dim> {
  static_assert(Axis < Dim, "Axis is out of range!");
  Vec<Num, Dim> e;
  e[Axis] = Num{1.0};
  return e;
}

/// Concatenate two vectors.
template<class Num, size_t Dim1, size_t Dim2>
constexpr auto vec_cat(const Vec<Num, Dim1>& a,
                       const Vec<Num, Dim2>& b) -> Vec<Num, Dim1 + Dim2> {
  Vec<Num, Dim1 + Dim2> r;
  for (size_t i = 0; i < Dim1; ++i) r[i] = a[i];
  for (size_t i = 0; i < Dim2; ++i) r[i + Dim1] = b[i];
  return r;
}

/// Extract the head part of the vector.
template<size_t HeadDim = 1, class Num, size_t Dim>
constexpr auto vec_head(const Vec<Num, Dim>& a) -> Vec<Num, HeadDim> {
  static_assert(HeadDim <= Dim, "Head dimension is out of range!");
  Vec<Num, HeadDim> r;
  for (size_t i = 0; i < HeadDim; ++i) r[i] = a[i];
  return r;
}

/// Extract the tail part of the vector.
template<size_t HeadDim = 1, class Num, size_t Dim>
constexpr auto vec_tail(const Vec<Num, Dim>& a) -> Vec<Num, Dim - HeadDim> {
  static_assert(HeadDim <= Dim, "Head dimension is out of range!");
  Vec<Num, Dim - HeadDim> r;
  for (size_t i = 0; i < Dim - HeadDim; ++i) r[i] = a[i + HeadDim];
  return r;
}

/// Element-wise vector cast.
/// @{
template<class To, class From, size_t Dim>
constexpr auto vec_cast(const Vec<From, Dim>& a) -> Vec<To, Dim> {
  Vec<To, Dim> r;
  TIT_IF_SIMD_CAST_AVALIABLE(From, To) {
    for (size_t i = 0; i < Vec<From, Dim>::RegCount; ++i) {
      r.reg(i) = simd::reg_cast<To>(a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = static_cast<To>(a[i]);
  return r;
}
template<template<class...> class To, class From, size_t Dim>
constexpr auto vec_cast(const Vec<From, Dim>& a)
    -> Vec<decltype(To{std::declval<From>()}), Dim> {
  return vec_cast<decltype(To{std::declval<From>()})>(a);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Algorithms
//

/// Element-wise minimum of two vectors.
template<class Num, size_t Dim>
constexpr auto minimum(const Vec<Num, Dim>& a,
                       const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::min(a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = std::min(a[i], b[i]);
  return r;
}

/// Element-wise maximum of two vectors.
template<class Num, size_t Dim>
constexpr auto maximum(const Vec<Num, Dim>& a,
                       const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::max(a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = std::max(a[i], b[i]);
  return r;
}

/// Filter a vector with a boolean mask.
template<class Num, size_t Dim>
constexpr auto filter(const VecMask<Num, Dim>& m,
                      const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::filter(m.reg(i), a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : Num{0};
  return r;
}

/// Select two vectors based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto select(const VecMask<Num, Dim>& m,
                      const Vec<Num, Dim>& a,
                      const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::select(m.reg(i), a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : b[i];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Arithmetic operations
//

/// Vector unary plus operation.
template<class Num, size_t Dim>
constexpr auto operator+(const Vec<Num, Dim>& a) noexcept
    -> const Vec<Num, Dim>& {
  return a;
}

/// Vector element-wise addition operation.
template<class Num, size_t Dim>
constexpr auto operator+(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) + b.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
  return r;
}

/// Vector element-wise addition with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator+=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) a.reg(i) += b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] += b[i];
  return a;
}

/// Vector element-wise negation operation.
template<class Num, size_t Dim>
constexpr auto operator-(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) r.reg(i) = -a.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = -a[i];
  return r;
}

/// Vector element-wise subtraction operation.
template<class Num, size_t Dim>
constexpr auto operator-(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) - b.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
  return r;
}

/// Vector element-wise subtraction with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator-=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) a.reg(i) -= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] -= b[i];
  return a;
}

/// Vector-scalar element-wise multiplication operation.
/// @{
template<class Num, size_t Dim>
constexpr auto operator*(const std::type_identity_t<Num>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  return b * a;
}
template<class Num, size_t Dim>
constexpr auto operator*(const Vec<Num, Dim>& a,
                         const std::type_identity_t<Num>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    const typename Vec<Num, Dim>::Reg b_reg(b);
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) * b_reg;
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
  return r;
}
/// @}

/// Vector-scalar element-wise multiplication with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a, const std::type_identity_t<Num>& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE(Num) {
    const typename Vec<Num, Dim>::Reg b_reg(b);
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) a.reg(i) *= b_reg;
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] *= b;
  return a;
}

/// Vector element-wise multiplication operation.
template<class Num, size_t Dim>
constexpr auto operator*(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) * b.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
  return r;
}

/// Vector element-wise multiplication with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) a.reg(i) *= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] *= b[i];
  return a;
}

/// Vector-scalar element-wise division operation.
template<class Num, size_t Dim>
constexpr auto operator/(const Vec<Num, Dim>& a,
                         const std::type_identity_t<Num>& b) -> Vec<Num, Dim> {
  if constexpr (Dim == 1) return a[0] / b;
  return a * inverse(b);
}

/// Vector-scalar element-wise division with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a, const std::type_identity_t<Num>& b)
    -> Vec<Num, Dim>& {
  if constexpr (Dim == 1) a[0] /= b;
  else a *= inverse(b);
  return a;
}

/// Vector element-wise division operation.
template<class Num, size_t Dim>
constexpr auto operator/(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) / b.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] / b[i];
  return r;
}

/// Vector element-wise division with assignment operation.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) a.reg(i) /= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] /= b[i];
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Mathematical functions
//

/// Compute the largest integer value not greater than vector element.
template<class Num, size_t Dim>
constexpr auto floor(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::floor(a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = floor(a[i]);
  return r;
}

/// Computes the nearest integer value to vector element.
template<class Num, size_t Dim>
constexpr auto round(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::round(a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = round(a[i]);
  return r;
}

/// Compute the least integer value not less than vector element.
template<class Num, size_t Dim>
constexpr auto ceil(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::ceil(a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = ceil(a[i]);
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Comparison operations
//

/// Vector element-wise "equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator==(const Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      m.reg(i) = a.reg(i) == b.reg(i);
    }
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] == b[i];
  return m;
}

/// Vector element-wise "not equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator!=(const Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      m.reg(i) = a.reg(i) != b.reg(i);
    }
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] != b[i];
  return m;
}

/// Vector element-wise "less than" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator<(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      m.reg(i) = a.reg(i) < b.reg(i);
    }
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] < b[i];
  return m;
}

/// Vector element-wise "less than or equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator<=(const Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      m.reg(i) = a.reg(i) <= b.reg(i);
    }
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] <= b[i];
  return m;
}

/// Vector element-wise "greater than" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator>(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  return b < a;
}

/// Vector element-wise "greater than or equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator>=(const Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> VecMask<Num, Dim> {
  return b <= a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Reductions
//

/// Sum of the vector elements.
template<class Num, size_t Dim>
constexpr auto sum(const Vec<Num, Dim>& a) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg += a.reg(i);
      auto r = simd::sum(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) r += a[i];
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i];
  return r;
}

/// Product of the vector elements.
template<class Num, size_t Dim>
constexpr auto prod(const Vec<Num, Dim>& a) -> Num {
  // This function is rarely used, so we do not bother with SIMD.
  Num r = a[0];
  for (size_t i = 1; i < Dim; ++i) r *= a[i];
  return r;
}

/// Minimal vector element.
template<class Num, size_t Dim>
constexpr auto min_value(const Vec<Num, Dim>& a) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) {
        r_reg = simd::min(r_reg, a.reg(i));
      }
      auto r = simd::min_value(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        r = std::min(r, a[i]);
      }
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = std::min(r, a[i]);
  return r;
}

/// Maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value(const Vec<Num, Dim>& a) -> Num {
  using std::max;
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) {
        r_reg = simd::max(r_reg, a.reg(i));
      }
      auto r = simd::max_value(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        r = std::max(r, a[i]);
      }
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = std::max(r, a[i]);
  return r;
}

/// Index of the minimal vector element.
template<class Num, size_t Dim>
constexpr auto min_value_index(const Vec<Num, Dim>& a) -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < Dim; ++i) {
    if (a[i] < a[ir]) ir = i;
  }
  return ir;
}

/// Index of the maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value_index(const Vec<Num, Dim>& a) -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < Dim; ++i) {
    if (a[i] > a[ir]) ir = i;
  }
  return ir;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Linear algebra
//

/// Vector dot product.
template<class Num, size_t Dim>
constexpr auto dot(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0) * b.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) {
        r_reg = simd::fma(a.reg(i), b.reg(i), r_reg);
      }
      auto r = simd::sum(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) r += a[i] * b[i];
      return r;
    }
  }
  auto r = a[0] * b[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i] * b[i];
  return r;
}

/// Vector squared norm.
template<class Num, size_t Dim>
constexpr auto norm2(const Vec<Num, Dim>& a) -> Num {
  return dot(a, a);
}

/// Vector norm.
template<class Num, size_t Dim>
constexpr auto norm(const Vec<Num, Dim>& a) -> Num {
  if constexpr (Dim == 1) return abs(a[0]);
  return sqrt(norm2(a));
}

/// Normalize a vector.
template<class Num, size_t Dim>
constexpr auto normalize(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  if constexpr (Dim == 1) return is_tiny(a[0]) ? Num{0} : sign(a[0]);
  const auto norm_sqr = norm2(a);
  constexpr auto eps_sqr = pow2(tiny_number_v<Num>);
  TIT_IF_SIMD_AVALIABLE(Num) {
    // Use blending since we are most likely deal with a non-zero vector.
    using Reg = typename Vec<Num, Dim>::Reg;
    const auto has_dir_reg = Reg(norm_sqr) >= Reg(eps_sqr);
    const Reg norm_recip_reg(rsqrt(norm_sqr));
    Vec<Num, Dim> r;
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = simd::filter(has_dir_reg, a.reg(i) * norm_recip_reg);
    }
    return r;
  }
  const auto has_dir = norm_sqr >= pow2(eps_sqr);
  return has_dir ? a * rsqrt(norm_sqr) : Vec<Num, Dim>{};
}

/// Is a vector approximately equal to another vector?
template<class Num, size_t Dim>
constexpr auto approx_equal_to(const Vec<Num, Dim>& a,
                               const Vec<Num, Dim>& b) -> bool {
  return norm2(a - b) <= pow2(tiny_number_v<Num>);
}

/// Vector cross product.
///
/// @returns 3D vector with a result of cross product.
template<class Num, size_t Dim>
  requires in_range_v<Dim, 1, 3>
constexpr auto cross(const Vec<Num, Dim>& a,
                     const Vec<Num, Dim>& b) -> Vec<Num, 3> {
  Vec<Num, 3> r{};
  if constexpr (Dim == 3) r[0] = a[1] * b[2] - a[2] * b[1];
  if constexpr (Dim == 3) r[1] = a[2] * b[0] - a[0] * b[2];
  if constexpr (Dim >= 2) r[2] = a[0] * b[1] - a[1] * b[0];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// IO operations.
//

/// Vector output operator.
template<class Stream, class Num, size_t Dim>
constexpr auto operator<<(Stream& stream, const Vec<Num, Dim>& a) -> Stream& {
  stream << a[0];
  for (size_t i = 1; i < Dim; ++i) stream << " " << a[i];
  return stream;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
