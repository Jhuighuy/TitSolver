/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec/vec_mask.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Column vector.
template<class Num, size_t Dim>
class Vec final {
public:

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::reg_size_for_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::reg_count_for_v<Num, Dim>;

  /// Type of the underlying register that is used.
  using Reg = simd::Reg<Num, RegSize>;

  /// Associated vector mask type.
  using VecMask = tit::VecMask<Num, Dim>;

  // TODO: to be removed!
  static constexpr auto num_rows = Dim;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() noexcept {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      regs_ = fill_array<RegCount>(Reg{});
      return;
    }
    col_ = fill_array<Dim>(Num{0});
  }

  /// Fill-initialize the vector with the value @p q.
  constexpr explicit(Dim > 1) Vec(Num q) noexcept {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      regs_ = fill_array<RegCount>(Reg(q));
      return;
    }
    col_ = fill_array<Dim>(q);
  }

  /// Construct a vector with elements @p qi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args> && ...)
  constexpr Vec(Args... qi) noexcept // NOSONAR
      : col_{make_array<Dim, Num>(qi...)} {}

#ifdef __clang__

  /// Move-construct the vector.
  constexpr Vec(Vec&&) = default;

  /// Copy-construct the vector.
  constexpr Vec(Vec const&) = default;

  /// Move-assign the vector.
  constexpr auto operator=(Vec&&) -> Vec& = default;

  /// Copy-assign the vector.
  constexpr auto operator=(Vec const&) -> Vec& = default;

  /// Destroy the vector.
  constexpr ~Vec() = default;

#endif

  /// Vector dimensionality.
  constexpr static auto dim() -> ssize_t {
    return Dim;
  }

  /// Element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> Num const& {
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
  auto reg(size_t i) const noexcept -> Reg const& {
    TIT_ASSERT(i < RegCount, "Register index is out of range!");
    return regs_[i];
  }
  /// @}

  // NOLINTEND(*-type-union-access)

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Vector arthimetic operations
  //

  /// Vector unary plus operation.
  friend constexpr auto operator+(Vec const& a) noexcept -> Vec const& {
    return a;
  }

  /// Vector addition operation.
  friend constexpr auto operator+(Vec const& a, Vec const& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) + b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
    return r;
  }

  /// Vector addition with assignment operation.
  friend constexpr auto operator+=(Vec& a, Vec const& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) += b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] += b[i];
    return a;
  }

  /// Vector negation operation.
  friend constexpr auto operator-(Vec const& a) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = -a.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = -a[i];
    return r;
  }

  /// Vector subtraction operation.
  friend constexpr auto operator-(Vec const& a, Vec const& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) - b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
    return r;
  }

  /// Vector subtraction with assignment operation.
  friend constexpr auto operator-=(Vec& a, Vec const& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) -= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] -= b[i];
    return a;
  }

  /// Vector-scalar multiplication operation.
  /// @{
  friend constexpr auto operator*(Num const& a, Vec const& b) -> Vec {
    return b * a;
  }
  friend constexpr auto operator*(Vec const& a, Num const& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      Reg const b_reg(b);
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b_reg;
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
    return r;
  }
  /// @}

  /// Vector-scalar multiplication with assignment operation.
  friend constexpr auto operator*=(Vec& a, Num const& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      Reg const b_reg(b);
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b_reg;
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] *= b;
    return a;
  }

  /// Element-wise vector multiplication operation.
  friend constexpr auto operator*(Vec const& a, Vec const& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
    return r;
  }

  /// Element-wise vector multiplication with assignment operation.
  friend constexpr auto operator*=(Vec& a, Vec const& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] *= b[i];
    return a;
  }

  /// Vector-scalar division operation.
  friend constexpr auto operator/(Vec const& a, Num const& b) -> Vec {
    if constexpr (Dim == 1) return a[0] / b;
    return a * inverse(b);
  }

  /// Vector-scalar division with assignment operation.
  friend constexpr auto operator/=(Vec& a, Num const& b) -> Vec& {
    if constexpr (Dim == 1) a[0] /= b;
    else a *= inverse(b);
    return a;
  }

  /// Element-wise vector division operation.
  friend constexpr auto operator/(Vec const& a, Vec const& b) -> Vec {
    Vec r;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) / b.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = a[i] / b[i];
    return r;
  }

  /// Element-wise vector division with assignment operation.
  friend constexpr auto operator/=(Vec& a, Vec const& b) -> Vec& {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) a.reg(i) /= b.reg(i);
      return a;
    }
    for (size_t i = 0; i < Dim; ++i) a[i] /= b[i];
    return a;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Vector comparison operations
  //

  /// Vector element-wise "equal to" comparison operation.
  friend constexpr auto operator==(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) == b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] == b[i];
    return m;
  }

  /// Vector element-wise "not equal to" comparison operation.
  friend constexpr auto operator!=(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) != b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] != b[i];
    return m;
  }

  /// Vector element-wise "less than" comparison operation.
  friend constexpr auto operator<(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) < b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] < b[i];
    return m;
  }

  /// Vector element-wise "less than or equal to" comparison operation.
  friend constexpr auto operator<=(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) <= b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] <= b[i];
    return m;
  }

  /// Vector element-wise "greater than" comparison operation.
  friend constexpr auto operator>(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) > b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] > b[i];
    return m;
  }

  /// Vector element-wise "greater than or equal to" comparison operation.
  friend constexpr auto operator>=(Vec const& a, Vec const& b) -> VecMask {
    VecMask m;
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) >= b.reg(i);
      return m;
    }
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i] >= b[i];
    return m;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Vector IO operations.
  //

  /// Vector output operator.
  template<class Stream>
  friend constexpr auto operator<<(Stream& stream, Vec const& a) -> Stream& {
    stream << a[0];
    for (size_t i = 1; i < Dim; ++i) stream << " " << a[i];
    return stream;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  union {
    std::array<Num, Dim> col_;
    std::array<Reg, RegCount> regs_;
  };

}; // class Vec<Num, Dim>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector casting
//

/// Element-wise vector cast.
template<class To, class Num, size_t Dim>
constexpr auto static_vec_cast(Vec<Num, Dim> const& a) -> Vec<To, Dim> {
  Vec<To, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = static_cast<To>(a[i]);
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector algorithms
//

/// Element-wise minimum of two vectors.
template<class Num, size_t Dim>
constexpr auto minimum(Vec<Num, Dim> const& a,
                       Vec<Num, Dim> const& b) -> Vec<Num, Dim> {
  using std::min;
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = min(a.reg(i), b.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = min(a[i], b[i]);
  return r;
}

/// Element-wise maximum of two vectors.
template<class Num, size_t Dim>
constexpr auto maximum(Vec<Num, Dim> const& a,
                       Vec<Num, Dim> const& b) -> Vec<Num, Dim> {
  using std::max;
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = max(a.reg(i), b.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = max(a[i], b[i]);
  return r;
}

/// Filter a vector with a boolean mask.
template<class Num, size_t Dim>
constexpr auto filter(VecMask<Num, Dim> const& m,
                      Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = filter(m.reg(i), a.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : Num{0};
  return r;
}

/// Select two vectors based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto select(VecMask<Num, Dim> const& m,
                      Vec<Num, Dim> const& a,
                      Vec<Num, Dim> const& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = blend(m.reg(i), a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : b[i];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector mathematical functions
//

/// Compute the largest integer value not greater than vector element.
template<class Num, size_t Dim>
constexpr auto floor(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = floor(a.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = floor(a[i]);
  return r;
}

/// Computes the nearest integer value to vector element.
template<class Num, size_t Dim>
constexpr auto round(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = round(a.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = round(a[i]);
  return r;
}

/// Compute the least integer value not less than vector element.
template<class Num, size_t Dim>
constexpr auto ceil(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = ceil(a.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = ceil(a[i]);
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector reductions
//

/// Sum of the vector components.
template<class Num, size_t Dim>
constexpr auto sum(Vec<Num, Dim> const& a) -> Num {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg += a.reg(i);
      auto r = sum(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) r += a[i];
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i];
  return r;
}

/// Minimal vector element.
template<class Num, size_t Dim>
constexpr auto min_value(Vec<Num, Dim> const& a) -> Num {
  using std::min;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg = min(r_reg, a.reg(i));
      auto r = min_value(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) r = min(r, a[i]);
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = min(r, a[i]);
  return r;
}

/// Maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value(Vec<Num, Dim> const& a) -> Num {
  using std::max;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg = max(r_reg, a.reg(i));
      auto r = max_value(r_reg);
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) r = max(r, a[i]);
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = max(r, a[i]);
  return r;
}

/// Index of the minimal vector element.
template<class Num, size_t Dim>
constexpr auto min_value_index(Vec<Num, Dim> const& a) -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < Dim; ++i) {
    if (a[i] < a[ir]) ir = i;
  }
  return ir;
}

/// Index of the maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value_index(Vec<Num, Dim> const& a) -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < Dim; ++i) {
    if (a[i] > a[ir]) ir = i;
  }
  return ir;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector algebraic functions
//

/// Vector dot product.
template<class Num, size_t Dim>
constexpr auto dot(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b) -> Num {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegSize = Vec<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = a.reg(0) * b.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) {
        r_reg = fma(a.reg(i), b.reg(i), r_reg);
      }
      auto r = sum(r_reg);
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
constexpr auto norm2(Vec<Num, Dim> const& a) -> Num {
  return dot(a, a);
}

/// Vector norm.
template<class Num, size_t Dim>
constexpr auto norm(Vec<Num, Dim> const& a) -> Num {
  if constexpr (Dim == 1) return abs(a[0]);
  return sqrt(norm2(a));
}

/// Normalize a vector.
template<class Num, size_t Dim>
constexpr auto normalize(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  if constexpr (Dim == 1) return is_tiny(a[0]) ? Num{0} : sign(a[0]);
  auto const norm_sqr = norm2(a);
  auto const eps_sqr = pow2(tiny_number_v<Num>);
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    // Use blending since we are most likely deal with a non-zero vector.
    using Reg = typename Vec<Num, Dim>::Reg;
    auto const has_dir_reg = Reg(norm_sqr) >= Reg(eps_sqr);
    Reg const norm_recip_reg(rsqrt(norm_sqr));
    Vec<Num, Dim> r;
    for (size_t i = 0; i < Vec<Num, Dim>::RegCount; ++i) {
      r.reg(i) = filter(has_dir_reg, a.reg(i) * norm_recip_reg);
    }
    return r;
  }
  auto const has_dir = norm_sqr >= pow2(eps_sqr);
  return has_dir ? a * rsqrt(norm_sqr) : Vec<Num, Dim>{};
}

/// Is a vector approximately equal to another vector?
template<class Num, size_t Dim>
constexpr auto approx_equal_to(Vec<Num, Dim> const& a,
                               Vec<Num, Dim> const& b) -> bool {
  return norm2(a - b) <= pow2(tiny_number_v<Num>);
}

/// Vector cross product.
///
/// @returns 3D vector with a result of cross product.
template<class Num, size_t Dim>
  requires in_range_v<Dim, 1, 3>
constexpr auto cross(Vec<Num, Dim> const& a,
                     Vec<Num, Dim> const& b) -> Vec<Num, 3> {
  Vec<Num, 3> r{};
  if constexpr (Dim == 3) r[0] = a[1] * b[2] - a[2] * b[1];
  if constexpr (Dim == 3) r[1] = a[2] * b[0] - a[0] * b[2];
  if constexpr (Dim >= 2) r[2] = a[0] * b[1] - a[1] * b[0];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
