/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Row-major square matrix.
template<class Num, size_t Dim>
class Mat final {
public:

  /// Matrix row type.
  using Row = Vec<Num, Dim>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Fill-initialize the matrix with zeroes.
  constexpr Mat() = default;

  /// Fill-initialize the matrix diagonal with the value @p q.
  constexpr explicit(Dim > 1) Mat(Num const& q) {
    for (size_t i = 0; i < Dim; ++i) {
      rows_[i] = {};
      rows_[i][i] = q;
    }
  }

  /// Initialize a matrix with rows.
  constexpr Mat(std::initializer_list<Row> rows) {
    TIT_ASSERT(rows.size() == Dim, "Invalid amount of rows!");
    std::ranges::copy(rows, rows_.begin());
  }

  /// Matrix row at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Row& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return rows_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> Row const& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return rows_[i];
  }
  /// @}

  /// Matrix element at index.
  /// @{
  constexpr auto operator[](size_t i, size_t j) noexcept -> Num& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    TIT_ASSERT(j < Dim, "Column index is out of range!");
    return rows_[i][j];
  }
  constexpr auto operator[](size_t i, size_t j) const noexcept -> Num const& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    TIT_ASSERT(j < Dim, "Column index is out of range!");
    return rows_[i][j];
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Matrix unary plus.
  friend constexpr auto operator+(Mat const& A) noexcept -> Mat const& {
    return A;
  }

  /// Matrix addition.
  friend constexpr auto operator+(Mat const& A, Mat const& B) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] + B[i];
    return R;
  }

  /// Matrix addition with assignment.
  friend constexpr auto operator+=(Mat& A, Mat const& B) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] += B[i];
    return A;
  }

  /// Matrix negation.
  friend constexpr auto operator-(Mat const& A) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = -A[i];
    return R;
  }

  /// Matrix subtraction.
  friend constexpr auto operator-(Mat const& A, Mat const& B) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] - B[i];
    return R;
  }

  /// Matrix subtraction with assignment.
  friend constexpr auto operator-=(Mat& A, Mat const& B) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] -= B[i];
    return A;
  }

  /// Matrix-scalar multiplication.
  /// @{
  friend constexpr auto operator*(Num const& a, Mat const& B) -> Mat {
    return B * a;
  }
  friend constexpr auto operator*(Mat const& A, Num const& b) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] * b;
    return R;
  }
  /// @}

  /// Matrix-scalar multiplication with assignment.
  friend constexpr auto operator*=(Mat& A, Num const& b) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] *= b;
    return A;
  }

  /// Matrix-vector multiplication.
  friend constexpr auto operator*(Mat const& A, Row const& b) -> Row {
    auto const T = transpose(A);
    auto r = T[0] * b[0];
    for (size_t i = 1; i < Dim; ++i) r += T[i] * b[i];
    return r;
  }

  /// Matrix-matrix multiplication.
  friend constexpr auto operator*(Mat const& A, Mat const& B) -> Mat {
    Mat R(0.0);
    for (size_t i = 0; i < Dim; ++i) {
      for (size_t j = 0; j < Dim; ++j) {
        for (size_t k = 0; k < Dim; ++k) R[i, j] += A[i, k] * B[k, j];
      }
    }
    return R;
  }

  /// Matrix-scalar division.
  friend constexpr auto operator/(Mat const& A, Num const& b) -> Mat {
    if constexpr (Dim == 1) return A[0, 0] / b;
    return A * inverse(b);
  }

  /// Matrix-scalar division with assignment.
  friend constexpr auto operator/=(Mat& A, Num const& b) -> Mat& {
    if constexpr (Dim == 1) A[0, 0] /= b;
    else A *= inverse(b);
    return A;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Matrix output operator.
  template<class Stream>
  friend constexpr auto operator<<(Stream& stream, Mat const& A) -> Stream& {
    stream << A[0];
    for (size_t i = 1; i < Dim; ++i) stream << " " << A[i];
    return stream;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::array<Row, Dim> rows_;

}; // class Mat

template<class Row, class... RestRows>
Mat(Row, RestRows...) -> Mat<vec_num_t<Row>, 1 + sizeof...(RestRows)>;

template<class Num, class... RestNums>
Mat(carr_ref_t<Num const, 1 + sizeof...(RestNums)>,
    carr_ref_t<RestNums const, 1 + sizeof...(RestNums)>...)
    -> Mat<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Make a diagonal matrix.
template<class Num, size_t Dim>
constexpr auto eye(Mat<Num, Dim> const& /*A*/,
                   Num const& q = Num{1.0}) -> Mat<Num, Dim> {
  return Mat<Num, Dim>(q);
}

/// Make a diagonal matrix.
template<class Num, size_t Dim>
constexpr auto diag(Vec<Num, Dim> const& d) -> Mat<Num, Dim> {
  Mat<Num, Dim> D{};
  for (size_t i = 0; i < Dim; ++i) D[i, i] = d[i];
  return D;
}

/// Extract matrix diagonal.
template<class Num, size_t Dim>
constexpr auto diag(Mat<Num, Dim> const& D) -> Vec<Num, Dim> {
  Vec<Num, Dim> d{};
  for (size_t i = 0; i < Dim; ++i) d[i] = D[i, i];
  return d;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Matrix trace (sum of the diagonal elements).
template<class Num, size_t Dim>
constexpr auto tr(Mat<Num, Dim> const& A) -> Num {
  auto r = A[0, 0];
  for (size_t i = 1; i < Dim; ++i) r += A[i, i];
  return r;
}

/// Product of the diagonal elements.
template<class Num, size_t Dim>
constexpr auto prod_diag(Mat<Num, Dim> const& A) -> Num {
  auto r = A[0, 0];
  for (size_t i = 1; i < Dim; ++i) r *= A[i, i];
  return r;
}

/// Vector outer product.
/// @{
template<class Num, size_t Dim>
constexpr auto outer(Vec<Num, Dim> const& a,
                     Vec<Num, Dim> const& b) -> Mat<Num, Dim> {
  Mat<Num, Dim> R;
  for (size_t i = 0; i < Dim; ++i) R[i] = a[i] * b;
  return R;
}
template<class Num, size_t Dim>
constexpr auto outer_sqr(Vec<Num, Dim> const& a) -> Mat<Num, Dim> {
  return outer(a, a);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
