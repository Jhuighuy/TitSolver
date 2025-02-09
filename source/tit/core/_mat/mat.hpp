/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/mat.hpp"
#pragma once

#include <algorithm>
#include <array>
#include <format>
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
  constexpr explicit(Dim > 1) Mat(const Num& q) {
    for (size_t i = 0; i < Dim; ++i) {
      rows_[i] = {};
      rows_[i][i] = q;
    }
  }

  /// Initialize a matrix with rows.
  constexpr explicit Mat(std::initializer_list<Row> rows) {
    TIT_ASSERT(rows.size() == Dim, "Invalid amount of rows!");
    std::ranges::copy(rows, rows_.begin());
  }

  /// Matrix rows array.
  constexpr auto rows(this auto&& self) noexcept -> auto&& {
    return TIT_FORWARD_LIKE(self, self.rows_);
  }

  /// Matrix row at index.
  constexpr auto operator[](this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return TIT_FORWARD_LIKE(self, self.rows_[i]);
  }

  /// Matrix element at index.
  constexpr auto operator[](this auto&& self, size_t i, size_t j) noexcept
      -> auto&& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    TIT_ASSERT(j < Dim, "Column index is out of range!");
    return TIT_FORWARD_LIKE(self, self.rows_[i][j]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Matrix unary plus.
  friend constexpr auto operator+(const Mat& A) noexcept -> Mat {
    return A;
  }

  /// Matrix addition.
  friend constexpr auto operator+(const Mat& A, const Mat& B) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] + B[i];
    return R;
  }

  /// Matrix addition with assignment.
  friend constexpr auto operator+=(Mat& A, const Mat& B) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] += B[i];
    return A;
  }

  /// Matrix negation.
  friend constexpr auto operator-(const Mat& A) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = -A[i];
    return R;
  }

  /// Matrix subtraction.
  friend constexpr auto operator-(const Mat& A, const Mat& B) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] - B[i];
    return R;
  }

  /// Matrix subtraction with assignment.
  friend constexpr auto operator-=(Mat& A, const Mat& B) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] -= B[i];
    return A;
  }

  /// Matrix-scalar multiplication.
  /// @{
  friend constexpr auto operator*(const Num& a, const Mat& B) -> Mat {
    return B * a;
  }
  friend constexpr auto operator*(const Mat& A, const Num& b) -> Mat {
    Mat R;
    for (size_t i = 0; i < Dim; ++i) R[i] = A[i] * b;
    return R;
  }
  /// @}

  /// Matrix-scalar multiplication with assignment.
  friend constexpr auto operator*=(Mat& A, const Num& b) -> Mat& {
    for (size_t i = 0; i < Dim; ++i) A[i] *= b;
    return A;
  }

  /// Matrix-vector multiplication.
  friend constexpr auto operator*(const Mat& A, const Row& b) -> Row {
    const auto T = transpose(A);
    auto r = T[0] * b[0];
    for (size_t i = 1; i < Dim; ++i) r += T[i] * b[i];
    return r;
  }

  /// Matrix-matrix multiplication.
  friend constexpr auto operator*(const Mat& A, const Mat& B) -> Mat {
    Mat R(0.0);
    for (size_t i = 0; i < Dim; ++i) {
      for (size_t j = 0; j < Dim; ++j) {
        for (size_t k = 0; k < Dim; ++k) R[i, j] += A[i, k] * B[k, j];
      }
    }
    return R;
  }

  /// Matrix-scalar division.
  friend constexpr auto operator/(const Mat& A, const Num& b) -> Mat {
    if constexpr (Dim == 1) return A[0, 0] / b;
    return A * inverse(b);
  }

  /// Matrix-scalar division with assignment.
  friend constexpr auto operator/=(Mat& A, const Num& b) -> Mat& {
    if constexpr (Dim == 1) A[0, 0] /= b;
    else A *= inverse(b);
    return A;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Matrix exact equality operator.
  friend constexpr auto operator==(const Mat& A, const Mat& B) noexcept
      -> bool {
    for (size_t i = 0; i < Dim; ++i) {
      if (any(A[i] != B[i])) return false;
    }
    return true;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::array<Row, Dim> rows_;

}; // class Mat

template<class Row, class... RestRows>
Mat(Row, RestRows...) -> Mat<vec_num_t<Row>, 1 + sizeof...(RestRows)>;

template<class Num, class... RestNums>
Mat(carr_ref_t<const Num, 1 + sizeof...(RestNums)>,
    carr_ref_t<const RestNums, 1 + sizeof...(RestNums)>...)
    -> Mat<Num, 1 + sizeof...(RestNums)>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Make a zero matrix.
template<class Num, size_t Dim>
constexpr auto zero(const Mat<Num, Dim>& /*A*/) -> Mat<Num, Dim> {
  return {};
}

/// Make a diagonal matrix.
template<class Num, size_t Dim>
constexpr auto eye(const Mat<Num, Dim>& /*A*/, const Num& q = Num{1.0})
    -> Mat<Num, Dim> {
  return Mat<Num, Dim>(q);
}

/// Make a diagonal matrix.
template<class Num, size_t Dim>
constexpr auto diag(const Vec<Num, Dim>& d) -> Mat<Num, Dim> {
  Mat<Num, Dim> D{};
  for (size_t i = 0; i < Dim; ++i) D[i, i] = d[i];
  return D;
}

/// Extract matrix diagonal.
template<class Num, size_t Dim>
constexpr auto diag(const Mat<Num, Dim>& D) -> Vec<Num, Dim> {
  Vec<Num, Dim> d{};
  for (size_t i = 0; i < Dim; ++i) d[i] = D[i, i];
  return d;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Matrix trace (sum of the diagonal elements).
template<class Num, size_t Dim>
constexpr auto tr(const Mat<Num, Dim>& A) -> Num {
  auto r = A[0, 0];
  for (size_t i = 1; i < Dim; ++i) r += A[i, i];
  return r;
}

/// Product of the diagonal elements.
template<class Num, size_t Dim>
constexpr auto prod_diag(const Mat<Num, Dim>& A) -> Num {
  auto r = A[0, 0];
  for (size_t i = 1; i < Dim; ++i) r *= A[i, i];
  return r;
}

/// Vector outer product.
/// @{
template<class Num, size_t Dim>
constexpr auto outer(const Vec<Num, Dim>& a, const Vec<Num, Dim>& b)
    -> Mat<Num, Dim> {
  Mat<Num, Dim> R;
  for (size_t i = 0; i < Dim; ++i) R[i] = a[i] * b;
  return R;
}
template<class Num, size_t Dim>
constexpr auto outer_sqr(const Vec<Num, Dim>& a) -> Mat<Num, Dim> {
  return outer(a, a);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Matrix approximate equality operator.
template<class Num, size_t Dim>
constexpr auto approx_equal_to(const Mat<Num, Dim>& A,
                               const Mat<Num, Dim>& B) noexcept -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    if (!approx_equal_to(A[i], B[i])) return false;
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a matrix into the output stream.
template<class Stream, class Num, size_t Dim>
constexpr void serialize(Stream& out, const Mat<Num, Dim>& mat) {
  serialize(out, mat.rows());
}

/// Deserialize a matrix from the input stream.
template<class Stream, class Num, size_t Dim>
constexpr auto deserialize(Stream& in, Mat<Num, Dim>& mat) -> bool {
  return deserialize(in, mat.rows());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// Matrix formatter.
template<class Num, tit::size_t Dim>
struct std::formatter<tit::Mat<Num, Dim>> {
  constexpr auto parse(auto& context) {
    return context.begin();
  }
  constexpr auto format(const tit::Mat<Num, Dim>& A, auto& context) const {
    auto out = std::format_to(context.out(), "{}", A[0]);
    for (tit::size_t i = 1; i < Dim; ++i) {
      out = std::format_to(out, " {}", A[i]);
    }
    return out;
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
