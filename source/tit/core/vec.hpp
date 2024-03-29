/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts> // IWYU pragma: keep
#include <functional>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Algebraic vector.
\******************************************************************************/
template<class Num, size_t Dim>
class Vec final {
public:

  /** Number of rows. */
  static constexpr auto num_rows = Dim;

private:

  std::array<Num, num_rows> col_;

public:

  /** Construct a vector with elements. */
  template<class... Args>
    requires (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args> && ...)
  constexpr explicit Vec(Args... qi) noexcept : col_{qi...} {}

  /** Fill-initialize the vector. */
  constexpr explicit Vec(Num q = Num{}) noexcept {
    col_.fill(q);
  }
  /** Fill-assign the vector. */
  constexpr auto operator=(Num q) noexcept -> Vec& {
    col_.fill(q);
    return *this;
  }

  /** Vector component at index. */
  /** @{ */
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> Num {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return col_[i];
  }
  /** @} */

}; // class Vec<Num, Dim>

template<class Num, class... RestNums>
Vec(Num, RestNums...) -> Vec<Num, 1 + sizeof...(RestNums)>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check if type is a specialization of vector. */
/** @{ */
template<class>
inline constexpr auto is_vec_v = false;
template<class Real, size_t Dim>
inline constexpr auto is_vec_v<Vec<Real, Dim>> = true;
/** @} */

/** Vector number type. */
template<class Vec>
  requires is_vec_v<Vec>
using vec_num_t = std::remove_cvref_t<decltype(std::declval<Vec>()[0])>;
/** Vector size. */
template<class Vec>
  requires is_vec_v<Vec>
inline constexpr auto vec_dim_v = Vec::num_rows;

/** Vector size. */
template<class Num, size_t Dim>
constexpr auto dim([[maybe_unused]] Vec<Num, Dim> a) noexcept {
  return static_cast<int>(Dim);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector input operator. */
template<class Stream, class Num, size_t Dim>
constexpr auto operator>>(Stream& stream, Vec<Num, Dim>& a) -> Stream& {
  for (size_t i = 0; i < a.num_rows; ++i) stream >> a[i];
  return stream;
}

/** Vector output operator. */
template<class Stream, class Num, size_t Dim>
constexpr auto operator<<(Stream& stream, Vec<Num, Dim> a) -> Stream& {
  stream << a[0];
  for (size_t i = 1; i < a.num_rows; ++i) stream << " " << a[i];
  return stream;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector unary plus. */
template<class Num, size_t Dim>
constexpr auto operator+(Vec<Num, Dim> a) noexcept {
  return a;
}

/** Vector addition. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator+(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<add_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] + b[i];
  return r;
}

/** Vector addition assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator+=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] += b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector negation. */
template<class Num, size_t Dim>
constexpr auto operator-(Vec<Num, Dim> a) noexcept {
  Vec<negate_result_t<Num>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = -a[i];
  return r;
}

/** Vector subtraction. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator-(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] - b[i];
  return r;
}

/** Vector subtraction assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator-=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] -= b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector multiplication. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(NumA a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a * b[i];
  return r;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] * b;
  return r;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] * b[i];
  return r;
}
/** @} */

/** Vector multiplication assignment. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*=(Vec<NumA, Dim>& a, NumB b) noexcept -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] *= b;
  return a;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] *= b[i];
  return a;
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector division. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] / b;
  return r;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] / b[i];
  return r;
}
/** @} */

/** Vector division assignment. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/=(Vec<NumA, Dim>& a, NumB b) noexcept -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] /= b;
  return a;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] /= b[i];
  return a;
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Cast vector components to a different type. */
template<class To, class From, size_t Dim>
constexpr auto static_vec_cast(Vec<From, Dim> a) noexcept {
  Vec<To, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = static_cast<To>(a[i]);
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute the largest integer value not greater than vector component. */
template<class Num, size_t Dim>
constexpr auto floor(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = floor(a[i]);
  return r;
}

/** Computes the nearest integer value to vector component. */
template<class Num, size_t Dim>
constexpr auto round(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = round(a[i]);
  return r;
}

/** Compute the least integer value not less than number. */
template<class Num, size_t Dim>
constexpr auto ceil(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = ceil(a[i]);
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Sum of the vector components. */
template<class Num, size_t Dim>
constexpr auto sum(Vec<Num, Dim> a) noexcept {
  add_result_t<Num> r{a[0]};
  for (size_t i = 1; i < a.num_rows; ++i) r += a[i];
  return r;
}

/** Minimal vector element. */
template<class Num, size_t Dim>
constexpr auto min_value(Vec<Num, Dim> a) noexcept -> Num {
  Num r{a[0]};
  for (size_t i = 1; i < a.num_rows; ++i) r = std::min(r, a[i]);
  return r;
}
/** Maximal vector element. */
template<class Num, size_t Dim>
constexpr auto max_value(Vec<Num, Dim> a) noexcept -> Num {
  Num r{a[0]};
  for (size_t i = 1; i < a.num_rows; ++i) r = std::max(r, a[i]);
  return r;
}

/** Minimal vector element index. */
template<class Num, size_t Dim>
constexpr auto argmin_value(Vec<Num, Dim> a) noexcept -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < a.num_rows; ++i) {
    if (a[i] < a[ir]) ir = i;
  }
  return ir;
}
/** Maximal vector element index. */
template<class Num, size_t Dim>
constexpr auto argmax_value(Vec<Num, Dim> a) noexcept -> size_t {
  size_t ir = 0;
  for (size_t i = 1; i < a.num_rows; ++i) {
    if (a[i] > a[ir]) ir = i;
  }
  return ir;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector dot product. */
template<class NumA, class NumB, size_t Dim>
constexpr auto dot(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  return sum(a * b);
}

/** Vector norm square. */
template<class Num, size_t Dim>
constexpr auto norm2(Vec<Num, Dim> a) noexcept {
  return dot(a, a);
}
/** Vector norm. */
template<class Num, size_t Dim>
constexpr auto norm(Vec<Num, Dim> a) noexcept {
  if constexpr (Dim == 1) return abs(a[0]);
  if constexpr (Dim == 2) return hypot(a[0], a[1]);
  if constexpr (Dim == 3) return hypot(a[0], a[1], a[2]);
  return sqrt(norm2(a));
}
/** Normalize vector. */
template<class Num, size_t Dim>
constexpr auto normalize(Vec<Num, Dim> a) noexcept {
  return safe_divide(a, norm(a));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector cross product.
 ** @returns 3D vector with a result of cross product. */
template<class NumA, class NumB, size_t Dim>
constexpr auto cross(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  static_assert(1 <= Dim && Dim <= 3);
  Vec<sub_result_t<mul_result_t<NumA, NumB>>, 3> r{};
  if constexpr (Dim == 3) r[0] = a[1] * b[2] - a[2] * b[1];
  if constexpr (Dim == 3) r[1] = a[2] * b[0] - a[0] * b[2];
  if constexpr (Dim >= 2) r[2] = a[0] * b[1] - a[1] * b[0];
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// A little bit of the 𝑒𝑥𝑝𝑟𝑒𝑠𝑠𝑖𝑜𝑛 𝑡𝑒𝑚𝑝𝑙𝑎𝑡𝑒𝑠 𝑚𝑎𝑔𝑖𝑐 happens here.

/** Vector comparison. */
template<std::copy_constructible Op, size_t Dim, class NumX, class NumY = NumX>
  requires std::invocable<Op, NumX, NumY>
struct VecCmp {
  /** Comparison operation. */
  Op op;
  /** Left operand. */
  Vec<NumX, Dim> x;
  /** Right operand. */
  Vec<NumY, Dim> y;
}; // class VecCmp

template<class Op, class NumX, class NumY, size_t Dim>
VecCmp(Op, Vec<NumX, Dim>, Vec<NumY, Dim>) -> VecCmp<Op, Dim, NumX, NumY>;

/** Standard comparison operator. */
template<class Op>
concept common_cmp_op =
    std::same_as<Op, std::equal_to<>> ||
    std::same_as<Op, std::not_equal_to<>> || //
    std::same_as<Op, std::less<>> || std::same_as<Op, std::less_equal<>> ||
    std::same_as<Op, std::greater<>> || std::same_as<Op, std::greater_equal<>>;

/** Vector component-wise comparison. */
/** @{ */
template<class NumX, class NumY, size_t Dim>
constexpr auto operator==(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::equal_to{}, x, y};
}
template<class NumX, class NumY, size_t Dim>
constexpr auto operator!=(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::not_equal_to{}, x, y};
}
template<class NumX, class NumY, size_t Dim>
constexpr auto operator<(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::less{}, x, y};
}
template<class NumX, class NumY, size_t Dim>
constexpr auto operator<=(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::less_equal{}, x, y};
}
template<class NumX, class NumY, size_t Dim>
constexpr auto operator>(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::greater{}, x, y};
}
template<class NumX, class NumY, size_t Dim>
constexpr auto operator>=(Vec<NumX, Dim> x, Vec<NumY, Dim> y) noexcept {
  return VecCmp{std::greater_equal{}, x, y};
}
/** @} */

/** Evaluate comparison result. */
template<class Op, class NumX, class NumY, size_t Dim>
constexpr auto eval(VecCmp<Op, Dim, NumX, NumY> cmp) noexcept {
  Vec<bool, Dim> r;
  auto const& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = op(x[i], y[i]);
  return r;
}

/** Merge vector with zero vector based on comparison result. */
template<class Op, class NumX, class NumY, class NumA, size_t Dim>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,
                     Vec<NumA, Dim> a) noexcept {
  Vec<NumA, Dim> r;
  auto const& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_rows; ++i) {
    // Supposed to be overridden by an intrinsic or optimized.
    r[i] = merge(op(x[i], y[i]), a[i]);
  }
  return r;
}
/** Merge two vectors bases on comparison result. */
template<class Op, class NumX, class NumY, class NumA, class NumB, size_t Dim>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp, //
                     Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  auto const& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_rows; ++i) {
    // Supposed to be overridden by an intrinsic or optimized.
    r[i] = merge(op(x[i], y[i]), a[i], b[i]);
  }
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Component-wise minimum. */
template<class Num, size_t Dim>
constexpr auto minimum(Vec<Num, Dim> a, Vec<Num, Dim> b) noexcept
    -> Vec<Num, Dim> {
  return merge(a < b, a, b);
}
/** Component-wise maximum. */
template<class Num, size_t Dim>
constexpr auto maximum(Vec<Num, Dim> a, Vec<Num, Dim> b) noexcept
    -> Vec<Num, Dim> {
  return merge(a > b, a, b);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Evaluate polynomial @f$ \sum c_k q^k @f$. */
template<class NumQ, class NumC, size_t Dim>
constexpr auto poly(NumQ q, Vec<NumC, Dim> ci) noexcept {
  add_result_t<mul_result_t<NumQ, NumC>> r{ci[Dim - 1]};
  for (ssize_t i = Dim - 2; i >= 0; --i) r = ci[i] + r * q;
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Enable SIMD.
// IWYU pragma: begin_exports
#include "tit/core/vec_simd.hpp"
// IWYU pragma: end_exports

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
