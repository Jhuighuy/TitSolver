/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <format>
#include <utility>

#include "tit/core/_vec/vec_mask.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/tuple.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Register specification for generic vectors.
template<class Num, size_t Dim>
struct VecSIMD {
  using Reg = Num;
  static constexpr auto RegSize = 1;
  static constexpr auto RegCount = Dim;
};

// Register specification for SIMD vectors.
template<simd::supported_type Num, size_t Dim>
struct VecSIMD<Num, Dim> {
  using Reg = simd::deduce_reg_t<Num, Dim>;
  static constexpr auto RegSize = simd::deduce_size_v<Num, Dim>;
  static constexpr auto RegCount = simd::deduce_count_v<Num, Dim>;
};

} // namespace impl

/// Column vector.
template<class Num, size_t Dim>
class Vec final {
public:

  /// Type of the underlying register that is used.
  using Reg = impl::VecSIMD<Num, Dim>::Reg;

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = impl::VecSIMD<Num, Dim>::RegSize;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = impl::VecSIMD<Num, Dim>::RegCount;

  /// Vector mask type.
  using VecMask = tit::VecMask<Num, Dim>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() noexcept {
    TIT_IF_SIMD_AVALIABLE(Num) {
      regs_ = {}, regs_.fill(Reg{}); // mark as active union member.
      return;
    }
    col_.fill(Num{0});
  }

  /// Fill-initialize the vector with the value @p q.
  constexpr explicit(Dim > 1) Vec(Num q) noexcept {
    TIT_IF_SIMD_AVALIABLE(Num) {
      regs_ = {}, regs_.fill(Reg(q)); // mark as active union member.
      return;
    }
    col_.fill(q);
  }

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

  /// Vector elements array.
  constexpr auto elems(this auto&& self) noexcept -> auto&& {
    return TIT_FORWARD_LIKE(self, self.col_);
  }

  /// Vector element at index.
  constexpr auto operator[](this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return TIT_FORWARD_LIKE(self, self.col_[i]);
  }

  /// Vector register at index.
  auto reg(this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < RegCount, "Register index is out of range!");
    return TIT_FORWARD_LIKE(self, self.regs_[i]);
  }

  // NOLINTEND(*-type-union-access)

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Vector unary plus operation.
  friend constexpr auto operator+(const Vec& a) noexcept -> Vec {
    return a;
  }

  /// Vector element-wise addition operation.
  friend constexpr auto operator+(const Vec& a, const Vec& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) + b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
    return r;
  }

  /// Vector element-wise addition with assignment operation.
  friend constexpr auto operator+=(Vec& a, const Vec& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) += b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] += b[i];
    return a;
  }

  /// Vector element-wise negation operation.
  friend constexpr auto operator-(const Vec& a) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = -a.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = -a[i];
    return r;
  }

  /// Vector element-wise subtraction operation.
  friend constexpr auto operator-(const Vec& a, const Vec& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) - b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
    return r;
  }

  /// Vector element-wise subtraction with assignment operation.
  friend constexpr auto operator-=(Vec& a, const Vec& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) -= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] -= b[i];
    return a;
  }

  /// Vector-scalar element-wise multiplication operation.
  /// @{
  friend constexpr auto operator*(const Num& a, const Vec& b) -> Vec {
    return b * a;
  }
  friend constexpr auto operator*(const Vec& a, const Num& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      const Reg b_reg(b);
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b_reg;
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
    return r;
  }
  /// @}

  /// Vector-scalar element-wise multiplication with assignment operation.
  friend constexpr auto operator*=(Vec& a, const Num& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE(Num) {
      const Reg b_reg(b);
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b_reg;
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] *= b;
    return a;
  }

  /// Vector element-wise multiplication operation.
  friend constexpr auto operator*(const Vec& a, const Vec& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
    return r;
  }

  /// Vector element-wise multiplication with assignment operation.
  friend constexpr auto operator*=(Vec& a, const Vec& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] *= b[i];
    return a;
  }

  /// Vector-scalar element-wise division operation.
  friend constexpr auto operator/(const Vec& a, const Num& b) -> Vec {
    if constexpr (Dim == 1) return a[0] / b;
    return a * inverse(b);
  }

  /// Vector-scalar element-wise division with assignment operation.
  friend constexpr auto operator/=(Vec& a, const Num& b) -> Vec& {
    if constexpr (Dim == 1) a[0] /= b;
    else a *= inverse(b);
    return a;
  }

  /// Vector element-wise division operation.
  friend constexpr auto operator/(const Vec& a, const Vec& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) / b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] / b[i];
    return r;
  }

  /// Vector element-wise division with assignment operation.
  friend constexpr auto operator/=(Vec& a, const Vec& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) /= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] /= b[i];
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Vector element-wise "equal to" comparison operation.
  friend constexpr auto operator==(const Vec& a, const Vec& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) == b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] == b[i];
    return m;
  }

  /// Vector element-wise "not equal to" comparison operation.
  friend constexpr auto operator!=(const Vec& a, const Vec& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) != b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] != b[i];
    return m;
  }

  /// Vector element-wise "less than" comparison operation.
  friend constexpr auto operator<(const Vec& a, const Vec& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) < b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] < b[i];
    return m;
  }

  /// Vector element-wise "less than or equal to" comparison operation.
  friend constexpr auto operator<=(const Vec& a, const Vec& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) <= b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] <= b[i];
    return m;
  }

  /// Vector element-wise "greater than" comparison operation.
  friend constexpr auto operator>(const Vec& a, const Vec& b) -> VecMask {
    return b < a;
  }

  /// Vector element-wise "greater than or equal to" comparison operation.
  friend constexpr auto operator>=(const Vec& a, const Vec& b) -> VecMask {
    return b <= a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  union {
    std::array<Num, Dim> col_{}; // mark as active union member.
    std::array<Reg, RegCount> regs_;
  };

}; // class Vec<SIMD>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Make a unit vector.
template<size_t Axis = 0, class Num, size_t Dim>
constexpr auto unit(const Vec<Num, Dim>& /*a*/, const Num& n = Num{1.0})
    -> Vec<Num, Dim> {
  static_assert(Axis < Dim, "Axis is out of range!");
  Vec<Num, Dim> e;
  e[Axis] = n;
  return e;
}

/// Concatenate two vectors.
template<class Num, size_t Dim1, size_t Dim2>
constexpr auto vec_cat(const Vec<Num, Dim1>& a, const Vec<Num, Dim2>& b)
    -> Vec<Num, Dim1 + Dim2> {
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

/// Element-wise minimum of two vectors.
template<class Num, size_t Dim>
constexpr auto minimum(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b)
    -> Vec<Num, Dim> {
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
constexpr auto maximum(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b)
    -> Vec<Num, Dim> {
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
constexpr auto filter(const VecMask<Num, Dim>& m, const Vec<Num, Dim>& a)
    -> Vec<Num, Dim> {
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

/// Sum of the vector elements.
template<class Num, size_t Dim>
constexpr auto sum(const Vec<Num, Dim>& a) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::sum(simd::take_n(Dim, a.reg(0)));
    } else {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg += a.reg(i);
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg += simd::take_n(Rem, a.reg(NFullRegs));
      }
      return simd::sum(r_reg);
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
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::min_value(
          simd::merge_n(Dim, a.reg(0), simd::broadcast(a.reg(0))));
    } else {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg = simd::min(r_reg, a.reg(i));
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg = simd::min(r_reg, simd::merge_n(Rem, a.reg(NFullRegs), r_reg));
      }
      return simd::min_value(r_reg);
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = std::min(r, a[i]);
  return r;
}

/// Maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value(const Vec<Num, Dim>& a) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::max_value(
          simd::merge_n(Dim, a.reg(0), simd::broadcast(a.reg(0))));
    } else {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg = simd::max(r_reg, a.reg(i));
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg = simd::max(r_reg, simd::merge_n(Rem, a.reg(NFullRegs), r_reg));
      }
      return simd::max_value(r_reg);
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

/// Vector dot product.
template<class Num, size_t Dim>
constexpr auto dot(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b) -> Num {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::sum(simd::take_n(Dim, a.reg(0) * b.reg(0)));
    } else {
      auto r_reg = a.reg(0) * b.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg = simd::fma(a.reg(i), b.reg(i), r_reg);
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg += simd::take_n(Rem, a.reg(NFullRegs) * b.reg(NFullRegs));
      }
      return simd::sum(r_reg);
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
  const auto norm_sqr = norm2(a);
  constexpr auto eps_sqr = pow2(tiny_v<Num>);
  TIT_IF_SIMD_AVALIABLE(Num) {
    // Use filtering since we are most likely deal with a non-zero vector.
    using Reg = typename Vec<Num, Dim>::Reg;
    const auto norm_recip_reg = simd::filter(Reg(norm_sqr) >= Reg(eps_sqr),
                                             Reg(inverse(sqrt(norm_sqr))));
    Vec<Num, Dim> r;
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = a.reg(i) * norm_recip_reg;
    }
    return r;
  }
  return (norm_sqr >= eps_sqr) ? a / sqrt(norm_sqr) : Vec<Num, Dim>{};
}

/// Is a vector approximately equal to another vector?
template<class Num, size_t Dim>
constexpr auto approx_equal_to(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b)
    -> bool {
  return norm2(a - b) <= pow2(tiny_v<Num>);
}

/// Vector cross product.
///
/// @returns 3D vector with a result of cross product.
template<class Num, size_t Dim>
  requires (in_range(Dim, 1, 3))
constexpr auto cross(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b)
    -> Vec<Num, 3> {
  Vec<Num, 3> r{};
  if constexpr (Dim == 3) r[0] = a[1] * b[2] - a[2] * b[1];
  if constexpr (Dim == 3) r[1] = a[2] * b[0] - a[0] * b[2];
  if constexpr (Dim >= 2) r[2] = a[0] * b[1] - a[1] * b[0];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a vector into the output stream.
template<class Stream, class Num, size_t Dim>
constexpr void serialize(Stream& out, const Vec<Num, Dim>& vec) {
  serialize(out, vec.elems());
}

/// Deserialize a vector from the input stream.
template<class Stream, class Num, size_t Dim>
constexpr auto deserialize(Stream& in, Vec<Num, Dim>& vec) -> bool {
  return deserialize(in, vec.elems());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Vector formatter.
template<class Num, tit::size_t Dim>
struct std::formatter<tit::Vec<Num, Dim>> {
  static constexpr auto parse(const std::format_parse_context& context) {
    return context.begin();
  }
  static constexpr auto format(const tit::Vec<Num, Dim>& a,
                               std::format_context& context) {
    auto out = std::format_to(context.out(), "{}", a[0]);
    for (tit::size_t i = 1; i < Dim; ++i) {
      out = std::format_to(out, " {}", a[i]);
    }
    return out;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
