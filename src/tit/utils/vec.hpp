/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <type_traits>

#include "tit/utils/math.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Algebraic vector.
\******************************************************************************/
template<class Num, dim_t Dim>
class Vec final : public std::array<Num, Dim> {
public:

  /** Fill-initialize the vector. */
  constexpr explicit Vec(Num value = Num{}) {
    this->fill(value);
  }

  /** Fill-assign the vector. */
  constexpr auto& operator=(Num value) {
    this->fill(value);
    return *this;
  }

}; // class Vec

/******************************************************************************\
 ** Point.
\******************************************************************************/
template<class Num, dim_t Dim>
using Point = Vec<Num, Dim>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector input operator. */
template<class Stream, class Num, dim_t Dim>
Stream& operator>>(Stream& stream, Vec<Num, Dim>& a) {
  for (dim_t i = 0; i < Dim; ++i) stream >> a[i];
  return stream;
}

/** Vector output operator. */
template<class Stream, class Num, dim_t Dim>
Stream& operator<<(Stream& stream, Vec<Num, Dim> a) {
  stream << a[0];
  for (dim_t i = 1; i < Dim; ++i) stream << " " << a[i];
  return stream;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector unary plus. */
template<class Num, dim_t Dim>
constexpr auto operator+(Vec<Num, Dim> a) noexcept {
  return a;
}

/** Vector addition. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator+(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<add_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = a[i] + b[i];
  return r;
}

/** Vector addition assignment. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator+=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] += b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector negation. */
template<class Num, dim_t Dim>
constexpr auto operator-(Vec<Num, Dim> a) noexcept {
  Vec<negate_result_t<Num>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = -a[i];
  return r;
}

/** Vector subtraction. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator-(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = a[i] - b[i];
  return r;
}

/** Vector subtraction assignment. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator-=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] -= b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector multiplication. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator*(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = a[i] * b;
  return r;
}
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator*(NumA a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = a * b[i];
  return r;
}
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator*(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = a[i] * b[i];
  return r;
}

/** Vector multiplication assignment. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator*=(Vec<NumA, Dim>& a, NumB b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] *= b;
  return a;
}
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator*=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] *= b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector division. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator/(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = divide(a[i], b);
  return r;
}
template<class NumA, class NumB, dim_t Dim>
constexpr auto operator/(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  for (dim_t i = 0; i < Dim; ++i) r[i] = divide(a[i], b[i]);
  return r;
}

/** Vector division assignment. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator/=(Vec<NumA, Dim>& a, NumB b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] = divide(a[i], b);
  return a;
}
template<class NumA, class NumB, dim_t Dim>
constexpr auto& operator/=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (dim_t i = 0; i < Dim; ++i) a[i] = divide(a[i], b[i]);
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector dot product. */
template<class NumA, class NumB, dim_t Dim>
constexpr auto dot(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  add_result_t<mul_result_t<NumA, NumB>> r{a[0] * b[0]};
  for (dim_t i = 1; i < Dim; ++i) r += a[i] * b[i];
  return r;
}

/** Vector norm square. */
template<class Num, dim_t Dim>
constexpr auto norm2(Vec<Num, Dim> a) noexcept {
  return dot(a, a);
}

/** Vector norm. */
template<class Num, dim_t Dim>
constexpr auto norm(Vec<Num, Dim> a) noexcept {
  if constexpr (Dim == 1) return abs(a[0]);
  if constexpr (Dim == 2) return hypot(a[0], a[1]);
  if constexpr (Dim == 3) return hypot(a[0], a[1], a[2]);
  return sqrt(norm2(a));
}

/** Normalize vector. */
template<class Num, dim_t Dim>
constexpr auto normalize(Vec<Num, Dim> a) noexcept {
  return safe_divide(a, norm(a));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
