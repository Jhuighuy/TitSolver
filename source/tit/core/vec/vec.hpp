/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts> // IWYU pragma: keep
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Column vector.
template<class Num, size_t Dim>
class Vec final {
public:

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() : Vec(Num{0}) {}

  /// Fill-initialize the vector with the value @p q.
  constexpr explicit(Dim > 1) Vec(Num const& q) : col_{fill_array<Dim>(q)} {}

  /// Construct a vector with elements @p qi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args &&> && ...)
  constexpr Vec(Args&&... qi) : col_{std::forward<Args>(qi)...} {} // NOSONAR

  /// Vector element at index.
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

  // TODO: to be removed!
  static constexpr auto num_rows = Dim;

private:

  std::array<Num, Dim> col_;

}; // class Vec<Num, Dim>

/// Column vector (SIMD specialization).
template<class Num, size_t Dim>
  requires simd::available_for<Num, Dim>
class Vec<Num, Dim> final {
public:

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::reg_size_for_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::reg_count_for_v<Num, Dim>;

  /// Type of the underlying register that is used.
  using Reg = simd::Reg<Num, RegSize>;

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector with zeroes.
  constexpr Vec() noexcept {
    if consteval {
      col_ = fill_array<Dim>(Num{0});
    } else {
      regs_ = fill_array<RegCount>(Reg(zeroinit));
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

  /// Construct a vector with elements @p qi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args> && ...)
  constexpr Vec(Args... qi) noexcept { // NOSONAR
    if consteval {
      col_ = {qi...};
    } else {
      auto const qs = make_array<RegSize * RegCount, Num>(qi...);
      auto const pack = [&]<size_t RegIndex>(std::index_sequence<RegIndex>) {
        return [&]<size_t... ElIndices>(std::index_sequence<ElIndices...>) {
          return Reg{qs[RegIndex * RegSize + ElIndices]...};
        }(std::make_index_sequence<RegSize>{});
      };
      [&]<size_t... RegIndices>(std::index_sequence<RegIndices...>) {
        regs_ = {pack(std::index_sequence<RegIndices>{})...};
      }(std::make_index_sequence<RegCount>{});
    }
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

  // TODO: to be removed!
  static constexpr auto num_rows = Dim;

private:

  union {
    std::array<Num, Dim> col_;
    std::array<Reg, RegCount> regs_;
  };

}; // class Vec<Num, Dim>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// TODO: I would like to remove this traits eventually.
namespace impl {
template<class>
struct vec_traits {
  static constexpr auto is_vec = false;
};
template<class Num, size_t Dim_>
struct vec_traits<Vec<Num, Dim_>> {
  static constexpr auto is_vec = true;
  using NumType = Num;
  static constexpr auto Dim = Dim_;
};
} // namespace impl

/// Check if type is a specialization of vector.
template<class Type>
inline constexpr auto is_vec_v = impl::vec_traits<Type>::is_vec;

/// Vector number type.
template<class Vec>
using vec_num_t = typename impl::vec_traits<Vec>::NumType;
/// Vector size.
template<class Vec>
inline constexpr auto vec_dim_v = impl::vec_traits<Vec>::Dim;

/// Vector size.
/// @returns Vector size as a signed integer.
template<class Num, size_t Dim>
constexpr auto dim([[maybe_unused]] Vec<Num, Dim> a) noexcept -> ssize_t {
  return static_cast<ssize_t>(Dim);
}

/// Element-wise vector cast.
template<class To, class Num, size_t Dim>
constexpr auto static_vec_cast(Vec<Num, Dim> const& a) -> Vec<To, Dim> {
  // Here I would like to avoid problems if `To` has no default constructor,
  // hence roll the loop explicitly.
  return [&]<size_t... Indices>(std::index_sequence<Indices...>) {
    return Vec{static_cast<To>(a[Indices])...};
  }(std::make_index_sequence<Dim>{});
}

/// Vector input operator.
template<class Stream, class Num, size_t Dim>
constexpr auto operator>>(Stream& stream, Vec<Num, Dim>& a) -> Stream& {
  for (size_t i = 0; i < Dim; ++i) stream >> a[i];
  return stream;
}

/// Vector output operator.
template<class Stream, class Num, size_t Dim>
constexpr auto operator<<(Stream& stream, Vec<Num, Dim> const& a) -> Stream& {
  stream << a[0];
  for (size_t i = 1; i < Dim; ++i) stream << " " << a[i];
  return stream;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector unary plus.
template<class Num, size_t Dim>
constexpr auto operator+(Vec<Num, Dim> const& a) noexcept
    -> Vec<Num, Dim> const& {
  return a;
}

/// Vector addition.
template<class Num, size_t Dim>
constexpr auto operator+(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) + b.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
  return r;
}

/// Vector addition with assignment.
template<class Num, size_t Dim>
constexpr auto operator+=(Vec<Num, Dim>& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) a.reg(i) += b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] += b[i];
  return a;
}

/// Vector negation.
template<class Num, size_t Dim>
constexpr auto operator-(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = -a.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = -a[i];
  return r;
}

/// Vector subtraction.
template<class Num, size_t Dim>
constexpr auto operator-(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) - b.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
  return r;
}

/// Vector subtraction with assignment.
template<class Num, size_t Dim>
constexpr auto operator-=(Vec<Num, Dim>& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) a.reg(i) -= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] -= b[i];
  return a;
}

/// Vector-scalar multiplication.
/// @{
template<class Num, size_t Dim>
constexpr auto operator*(std::type_identity_t<Num> const& a,
                         Vec<Num, Dim> const& b) -> Vec<Num, Dim> {
  return b * a;
}
template<class Num, size_t Dim>
constexpr auto operator*(Vec<Num, Dim> const& a,
                         std::type_identity_t<Num> const& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    typename Vec<Num, Dim>::Reg const b_reg(b);
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b_reg;
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
  return r;
}
/// @}

/// Vector-scalar multiplication with assignment.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a, std::type_identity_t<Num> const& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    typename Vec<Num, Dim>::Reg const b_reg(b);
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b_reg;
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] *= b;
  return a;
}

/// Element-wise vector multiplication.
template<class Num, size_t Dim>
constexpr auto operator*(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) * b.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
  return r;
}

/// Element-wise vector multiplication with assignment.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) a.reg(i) *= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] *= b[i];
  return a;
}

/// Vector-scalar division.
template<class Num, size_t Dim>
constexpr auto operator/(Vec<Num, Dim> const& a,
                         std::type_identity_t<Num> const& b) -> Vec<Num, Dim> {
  if constexpr (Dim == 1) return a[0] / b;
  return a * inverse(b);
}

/// Vector-scalar division with assignment.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a, std::type_identity_t<Num> const& b)
    -> Vec<Num, Dim>& {
  if constexpr (Dim == 1) a[0] /= b;
  else a *= inverse(b);
  return a;
}

/// Element-wise vector division.
template<class Num, size_t Dim>
constexpr auto operator/(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = a.reg(i) / b.reg(i);
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] / b[i];
  return r;
}

/// Element-wise vector division with assignment.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim>& {
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) a.reg(i) /= b.reg(i);
    return a;
  }
  for (size_t i = 0; i < Dim; ++i) a[i] /= b[i];
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Element-wise absolute values of a vector.
template<class Num, size_t Dim>
constexpr auto abs(Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) r.reg(i) = abs(a.reg(i));
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = abs(a[i]);
  return r;
}

/// Element-wise absolute difference of two vectors.
template<class Num, size_t Dim>
constexpr auto abs_delta(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) {
      r.reg(i) = abs_delta(a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = abs(a[i] - b[i]);
  return r;
}

/// Element-wise minimum of two vectors.
template<class Num, size_t Dim>
constexpr auto minimum(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
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
constexpr auto maximum(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
      constexpr auto TailBegin = FullRegCount * RegSize;
      for (size_t i = TailBegin; i < Dim; ++i) r += a[i];
      return r;
    }
  }
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i];
  return r;
}

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
      constexpr auto TailBegin = FullRegCount * RegSize;
      for (size_t i = TailBegin; i < Dim; ++i) r += a[i] * b[i];
      return r;
    }
  }
  auto r = a[0] * b[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i] * b[i];
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
      constexpr auto TailBegin = FullRegCount * RegSize;
      for (size_t i = TailBegin; i < Dim; ++i) r = min(r, a[i]);
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
      constexpr auto TailBegin = FullRegCount * RegSize;
      for (size_t i = TailBegin; i < Dim; ++i) r = max(r, a[i]);
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

/// Vector norm square.
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
  if constexpr (Dim == 1) return Vec{is_small(a[0]) ? Num{0} : sign(a[0])};
  auto const norm_sqr = norm2(a), eps_sqr = pow2(small_number_v<Num>);
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    // Use blending since we are most likely deal with a non-zero vector.
    using Reg = typename Vec<Num, Dim>::Reg;
    auto const has_dir_reg = Reg(norm_sqr) >= Reg(eps_sqr);
    Vec<Num, Dim> r;
    Reg const norm_recip_reg(rsqrt(norm_sqr));
    for (size_t i = 0; i < r.RegCount; ++i) {
      r.reg(i) = blend_zero(has_dir_reg, a.reg(i) * norm_recip_reg);
    }
    return r;
  }
  auto const has_dir = norm_sqr >= pow2(eps_sqr);
  return has_dir ? a * rsqrt(norm_sqr) : Vec<Num, Dim>{};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector cross product.
/// @returns 3D vector with a result of cross product.
template<class Num, size_t Dim>
  requires in_range_v<Dim, 1, 3>
constexpr auto cross(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, 3> {
  Vec<Num, 3> r{};
  if constexpr (Dim == 3) r[0] = a[1] * b[2] - a[2] * b[1];
  if constexpr (Dim == 3) r[1] = a[2] * b[0] - a[0] * b[2];
  if constexpr (Dim >= 2) r[2] = a[0] * b[1] - a[1] * b[0];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
