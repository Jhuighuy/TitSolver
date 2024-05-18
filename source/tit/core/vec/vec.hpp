/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
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
  constexpr explicit(Dim > 1) Vec(const Num& q) : col_{fill_array<Dim>(q)} {}

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
  constexpr auto operator[](size_t i) const noexcept -> const Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  /// @}

private:

  std::array<Num, Dim> col_;

}; // class Vec<Num, Dim>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Is the type a specialization of a vector type?
/// @{
template<class>
inline constexpr bool is_vec_v = false;
template<class Num, size_t Dim>
inline constexpr bool is_vec_v<Vec<Num, Dim>> = true;
/// @}

/// Number type of the vector.
template<class Vec>
  requires is_vec_v<Vec>
using vec_num_t = std::remove_cvref_t<decltype(std::declval<Vec>()[0])>;

/// Dimensionality of the vector.
template<class Num, size_t Dim>
constexpr auto dim(const Vec<Num, Dim>& /*unused*/) noexcept -> ssize_t {
  return Dim;
}

/// Dimensionality of the vector.
template<class Vec>
  requires is_vec_v<Vec>
inline constexpr size_t vec_dim_v = dim(Vec{});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector output operator.
template<class Stream, class Num, size_t Dim>
constexpr auto operator<<(Stream& stream, const Vec<Num, Dim>& a) -> Stream& {
  stream << a[0];
  for (size_t i = 1; i < Dim; ++i) stream << " " << a[i];
  return stream;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Element-wise vector cast.
template<class To, class Num, size_t Dim>
constexpr auto static_vec_cast(const Vec<Num, Dim>& a) -> Vec<To, Dim> {
  // Here I would like to avoid problems if `To` has no default constructor,
  // hence roll the loop explicitly.
  return [&]<size_t... Indices>(std::index_sequence<Indices...>) {
    return Vec{static_cast<To>(a[Indices])...};
  }(std::make_index_sequence<Dim>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector unary plus.
template<class Num, size_t Dim>
constexpr auto operator+(const Vec<Num, Dim>& a) noexcept
    -> const Vec<Num, Dim>& {
  return a;
}

/// Vector addition.
template<class Num, size_t Dim>
constexpr auto operator+(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
  return r;
}

/// Vector addition with assignment.
template<class Num, size_t Dim>
constexpr auto operator+=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  for (size_t i = 0; i < Dim; ++i) a[i] += b[i];
  return a;
}

/// Vector negation.
template<class Num, size_t Dim>
constexpr auto operator-(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = -a[i];
  return r;
}

/// Vector subtraction.
template<class Num, size_t Dim>
constexpr auto operator-(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
  return r;
}

/// Vector subtraction with assignment.
template<class Num, size_t Dim>
constexpr auto operator-=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  for (size_t i = 0; i < Dim; ++i) a[i] -= b[i];
  return a;
}

/// Vector-scalar multiplication.
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
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
  return r;
}
/// @}

/// Vector-scalar multiplication with assignment.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a, const std::type_identity_t<Num>& b)
    -> Vec<Num, Dim>& {
  for (size_t i = 0; i < Dim; ++i) a[i] *= b;
  return a;
}

/// Element-wise vector multiplication.
template<class Num, size_t Dim>
constexpr auto operator*(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
  return r;
}

/// Element-wise vector multiplication with assignment.
template<class Num, size_t Dim>
constexpr auto operator*=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  for (size_t i = 0; i < Dim; ++i) a[i] *= b[i];
  return a;
}

/// Vector-scalar division.
template<class Num, size_t Dim>
constexpr auto operator/(const Vec<Num, Dim>& a,
                         const std::type_identity_t<Num>& b) -> Vec<Num, Dim> {
  if constexpr (Dim == 1) return a[0] / b;
  return a * inverse(b);
}

/// Vector-scalar division with assignment.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a, const std::type_identity_t<Num>& b)
    -> Vec<Num, Dim>& {
  if constexpr (Dim == 1) a[0] /= b;
  else a *= inverse(b);
  return a;
}

/// Element-wise vector division.
template<class Num, size_t Dim>
constexpr auto operator/(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = a[i] / b[i];
  return r;
}

/// Element-wise vector division with assignment.
template<class Num, size_t Dim>
constexpr auto operator/=(Vec<Num, Dim>& a,
                          const Vec<Num, Dim>& b) -> Vec<Num, Dim>& {
  for (size_t i = 0; i < Dim; ++i) a[i] /= b[i];
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Element-wise absolute values of a vector.
template<class Num, size_t Dim>
constexpr auto abs(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = abs(a[i]);
  return r;
}

/// Element-wise absolute difference of two vectors.
template<class Num, size_t Dim>
constexpr auto abs_delta(const Vec<Num, Dim>& a,
                         const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = abs(a[i] - b[i]);
  return r;
}

/// Element-wise minimum of two vectors.
template<class Num, size_t Dim>
constexpr auto minimum(const Vec<Num, Dim>& a,
                       const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  using std::min;
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = min(a[i], b[i]);
  return r;
}

/// Element-wise maximum of two vectors.
template<class Num, size_t Dim>
constexpr auto maximum(const Vec<Num, Dim>& a,
                       const Vec<Num, Dim>& b) -> Vec<Num, Dim> {
  using std::max;
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = max(a[i], b[i]);
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the largest integer value not greater than vector element.
template<class Num, size_t Dim>
constexpr auto floor(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = floor(a[i]);
  return r;
}

/// Computes the nearest integer value to vector element.
template<class Num, size_t Dim>
constexpr auto round(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = round(a[i]);
  return r;
}

/// Compute the least integer value not less than vector element.
template<class Num, size_t Dim>
constexpr auto ceil(const Vec<Num, Dim>& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = ceil(a[i]);
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Sum of the vector components.
template<class Num, size_t Dim>
constexpr auto sum(const Vec<Num, Dim>& a) -> Num {
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i];
  return r;
}

/// Vector dot product.
template<class Num, size_t Dim>
constexpr auto dot(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b) -> Num {
  auto r = a[0] * b[0];
  for (size_t i = 1; i < Dim; ++i) r += a[i] * b[i];
  return r;
}

/// Minimal vector element.
template<class Num, size_t Dim>
constexpr auto min_value(const Vec<Num, Dim>& a) -> Num {
  using std::min;
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = min(r, a[i]);
  return r;
}

/// Maximal vector element.
template<class Num, size_t Dim>
constexpr auto max_value(const Vec<Num, Dim>& a) -> Num {
  using std::max;
  auto r = a[0];
  for (size_t i = 1; i < Dim; ++i) r = max(r, a[i]);
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

/// Vector norm square.
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
  if constexpr (Dim == 1) return Vec{is_tiny(a[0]) ? Num{0} : sign(a[0])};
  const auto norm_sqr = norm2(a);
  const auto eps_sqr = pow2(tiny_number_v<Num>);
  const auto has_dir = norm_sqr >= pow2(eps_sqr);
  return has_dir ? a * rsqrt(norm_sqr) : Vec<Num, Dim>{};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector cross product.
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

} // namespace tit
