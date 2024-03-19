/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Algebraic square matrix.
\******************************************************************************/
template<class Num, size_t Dim>
class Mat final {
public:

  /** Number of rows. */
  static constexpr auto num_rows = Dim;

  /** Matrix row type. */
  using Row = Vec<Num, Dim>;

private:

  std::array<Row, Dim> rows_;

public:

  /** Construct a matrix with rows. */
  template<class... Args>
    requires (sizeof...(Args) == Dim) &&
             (std::constructible_from<Row, Args> && ...)
  constexpr explicit Mat(Args... ri) noexcept : rows_{ri...} {}

  /** Construct a scalar matrix. */
  constexpr explicit Mat(Num q = Num{}) noexcept {
    *this = q;
  }
  /** Assign scalar matrix. */
  constexpr auto operator=(Num q) noexcept -> Mat& {
    for (size_t i = 0; i < num_rows; ++i) {
      for (size_t j = 0; j < num_rows; ++j) {
        (*this)[i, j] = (i == j) ? q : Num{};
      }
    }
    return *this;
  }

  /** Matrix row at index. */
  /** @{ */
  constexpr auto operator[](size_t i) noexcept -> Row& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return rows_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> Row {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return rows_[i];
  }
  /** @} */

  /** Matrix element at index. */
  /** @{ */
  constexpr auto operator[](size_t i, size_t j) noexcept -> Num& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    TIT_ASSERT(j < num_rows, "Column index is out of range.");
    return rows_[i][j];
  }
  constexpr auto operator[](size_t i, size_t j) const noexcept -> Num {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    TIT_ASSERT(j < num_rows, "Column index is out of range.");
    return rows_[i][j];
  }
  /** @} */

}; // class Mat

// TODO: We cannot yet construct matrices like `Mat{{1,0},{0,1}}`.
template<class Row, class... RestRows>
Mat(Row, RestRows...) -> Mat<vec_num_t<Row>, 1 + sizeof...(RestRows)>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Matrix input operator. */
template<class Stream, class Num, size_t Dim>
constexpr auto operator>>(Stream& stream, Mat<Num, Dim>& a) -> Stream& {
  for (size_t i = 0; i < a.num_rows; ++i) stream >> a[i];
  return stream;
}

/** Vector output operator. */
template<class Stream, class Num, size_t Dim>
constexpr auto operator<<(Stream& stream, Mat<Num, Dim> a) -> Stream& {
  stream << a[0];
  for (size_t i = 1; i < a.num_rows; ++i) stream << " " << a[i];
  return stream;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Matrix unary plus. */
template<class Num, size_t Dim>
constexpr auto operator+(Mat<Num, Dim> const& a) noexcept {
  return a;
}

/** Matrix addition. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator+(Mat<NumA, Dim> const& a,
                         Mat<NumB, Dim> const& b) noexcept {
  Mat<add_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] + b[i];
  return r;
}

/** Matrix addition assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator+=(Mat<NumA, Dim>& a, Mat<NumB, Dim> const& b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] += b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Matrix negation. */
template<class Num, size_t Dim>
constexpr auto operator-(Mat<Num, Dim> const& a) noexcept {
  Mat<negate_result_t<Num>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = -a[i];
  return r;
}

/** Matrix subtraction. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator-(Mat<NumA, Dim> const& a,
                         Mat<NumB, Dim> const& b) noexcept {
  Mat<sub_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] - b[i];
  return r;
}

/** Matrix subtraction assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator-=(Mat<NumA, Dim>& a, Mat<NumB, Dim> const& b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] -= b[i];
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Matrix-scalar multiplication. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(NumA a, Mat<NumB, Dim> const& b) noexcept {
  Mat<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a * b[i];
  return r;
}
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(Mat<NumA, Dim> const& a, NumB b) noexcept {
  Mat<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] * b;
  return r;
}
/** @} */

/** Matrix-scalar multiplication assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*=(Mat<NumA, Dim>& a, NumB b) noexcept -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] *= b;
  return a;
}

/** Matrix-scalar division. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/(Mat<NumA, Dim> const& a, NumB b) noexcept {
  Mat<div_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] / b;
  return r;
}

/** Matrix-scalar division assignment. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator/=(Mat<NumA, Dim>& a, NumB b) noexcept -> auto& {
  for (size_t i = 0; i < a.num_rows; ++i) a[i] /= b;
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Matrix-vector multiplication. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(Mat<NumA, Dim> const& a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = dot(a[i], b);
  return r;
}
/** Matrix-matrix multiplication. */
template<class NumA, class NumB, size_t Dim>
constexpr auto operator*(Mat<NumA, Dim> const& a,
                         Mat<NumB, Dim> const& b) noexcept {
  Mat<mul_result_t<NumA, NumB>, Dim> r(0.0);
  for (size_t i = 0; i < r.num_rows; ++i) {
    for (size_t j = 0; j < r.num_rows; ++j) {
      for (size_t k = 0; k < r.num_rows; ++k) r[i, j] += a[i, k] * b[k, j];
    }
  }
  return r;
}

/** Vector outer product. */
template<class NumA, class NumB, size_t Dim>
constexpr auto outer(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Mat<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_rows; ++i) r[i] = a[i] * b;
  return r;
}

/** Matrix transpose. */
template<class Num, size_t Dim>
constexpr auto transpose(Mat<Num, Dim> a) noexcept {
  for (size_t i = 0; i < a.num_rows; ++i) {
    for (size_t j = 0; j < i; ++j) std::swap(a[i, j], a[j, i]);
  }
  return a;
}

/** Matrix trace. */
template<class Num, size_t Dim>
constexpr auto tr(Mat<Num, Dim> a) noexcept {
  add_result_t<Num> r = a[0, 0];
  for (size_t i = 1; i < a.num_rows; ++i) r += a[i, i];
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Matrix inversion.
\******************************************************************************/
template<class Num, size_t Dim>
class MatInv final {
private:

  Mat<Num, Dim> _l, _u;
  Num _det;

public:

  /** Construct matrix inversion. */
  constexpr explicit MatInv(Mat<Num, Dim> const& a) noexcept
      : _l(Num{1.0}), _u(Num{0.0}) {
    // Compute factors.
    for (size_t i = 0; i < a.num_rows; ++i) {
      for (size_t j = 0; j < i; ++j) {
        _l[i, j] = a[i, j];
        for (size_t k = 0; k < j; ++k) _l[i, j] -= _l[i, k] * _u[k, j];
        _l[i, j] /= _u[j, j];
      }
      for (size_t j = i; j < a.num_rows; ++j) {
        _u[i, j] = a[i, j];
        for (size_t k = 0; k < i; ++k) _u[i, j] -= _l[i, k] * _u[k, j];
      }
    }
    // Compute "determinant".
    _det = abs(_l[0, 0] * _u[0, 0]);
    for (size_t i = 1; i < a.num_rows; ++i) {
      _det = std::min(_det, abs(_l[i, i] * _u[i, i]));
    }
  }

  constexpr auto tdet() const noexcept {
    auto det = _l[0, 0] * _u[0, 0];
    for (size_t i = 1; i < _l.num_rows; ++i) {
      det *= _l[i, i] * _u[i, i];
    }
    return det;
  }

  /** Determinant of the matrix. */
  constexpr auto det() const noexcept {
    return _det;
  }

  /** Multiply by inverse matrix.
   ** @param x Vector or matrix of correct size. */
  template<class Obj>
  constexpr auto operator()(Obj x) const noexcept -> Obj {
    TIT_ASSERT(!is_zero(det()), "Matrix must be non-singular.");
    // "Divide" by L.
    for (size_t i = 0; i < x.num_rows; ++i) {
      for (size_t j = 0; j < i; ++j) x[i] -= _l[i, j] * x[j];
      x[i] /= _l[i, i];
    }
    // "Divide" by U.
    for (ssize_t i = x.num_rows - 1; i >= 0; --i) {
      for (size_t j = i + 1; j < x.num_rows; ++j) x[i] -= _u[i, j] * x[j];
      x[i] /= _u[i, i];
    }
    return x;
  }
  /** Evaluate inverse matrix. */
  constexpr auto operator()() const noexcept -> Mat<Num, Dim> {
    TIT_ASSERT(!is_zero(det()), "Matrix must be non-singular.");
    return (*this)(Mat<Num, Dim>(1.0));
  }

}; // class MatInv

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
