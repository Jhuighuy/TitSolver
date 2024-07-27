/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/mat.hpp"
#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/enum_utils.hpp"
#include "tit/core/mat/mat.hpp"
#include "tit/core/mat/traits.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Matrix part specification.
enum class MatPart : uint8_t {
  diag = 1 << 0,             ///< Diagonal.
  unit = 1 << 1,             ///< Unit diagonal.
  lower = 1 << 2,            ///< Lower triangular.
  upper = 1 << 3,            ///< Upper triangular.
  lower_diag = lower | diag, ///< Lower triangular and diagonal.
  upper_diag = upper | diag, ///< Upper triangular and diagonal.
  lower_unit = lower | unit, ///< Lower triangular and a unit diagonal.
  upper_unit = upper | unit, ///< Upper triangular and a unit diagonal.
  transposed = 1 << 7,       ///< Transpose the matrix.
};

template<>
inline constexpr bool is_flags_enum_v<MatPart> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Matrix part element getter.
template<MatPart Part>
struct MatPartAt {
  template<class Num, size_t Dim>
  static constexpr auto operator()(const Mat<Num, Dim>& A,
                                   size_t i,
                                   size_t j) -> Num {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    TIT_ASSERT(j < Dim, "Column index is out of range!");
    using enum MatPart;
    if constexpr (Part & unit) {
      static_assert(!(Part & diag), "Only one diagonal part bit can be set!");
      if (i == j) return Num{1.0};
    }
    bool ok = false;
    if constexpr (Part & diag) ok |= (i == j);
    if constexpr (Part & lower) ok |= (i > j);
    if constexpr (Part & upper) ok |= (i < j);
    if (!ok) return Num{0.0};
    if constexpr (Part & transposed) return A[j, i];
    return A[i, j];
  }
};

/// Get the matrix part element.
template<MatPart Part>
inline constexpr auto part_at = MatPartAt<Part>{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Copy the matrix part.
template<MatPart Part, class Num, size_t Dim>
constexpr auto copy_part(const Mat<Num, Dim>& A) -> Mat<Num, Dim> {
  Mat<Num, Dim> R;
  for (size_t i = 0; i < Dim; ++i) {
    for (size_t j = 0; j < Dim; ++j) R[i, j] = part_at<Part>(A, i, j);
  }
  return R;
}

/// Transpose the matrix.
template<class Mat>
constexpr auto transpose(const Mat& A) -> Mat {
  using enum MatPart;
  return copy_part<lower | diag | upper | transposed>(A);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partially solve matrix equation inplace:
/// `x := copy_part<Part>(A)^-1 * x`.
template<MatPart Part,
         class Num,
         size_t Dim,
         mat_multiplier<Mat<Num, Dim>> Mult>
constexpr void part_solve_inplace(const Mat<Num, Dim>& A, Mult& x) {
  using enum MatPart;
  constexpr MatPartAt<Part> at{};
  static_assert(Part & (diag | unit), "Diagonal bit must be set!");
  if constexpr (Part & lower) {
    static_assert(!(Part & upper), "Only one triangular part bit must be set!");
    for (size_t i = 0; i < Dim; ++i) {
      for (size_t j = 0; j < i; ++j) x[i] -= at(A, i, j) * x[j];
      x[i] /= at(A, i, i);
    }
  } else if constexpr (Part & upper) {
    for (ssize_t i = Dim - 1; i >= 0; --i) {
      for (size_t j = i + 1; j < Dim; ++j) x[i] -= at(A, i, j) * x[j];
      x[i] /= at(A, i, i);
    }
  } else if constexpr (Part & diag) {
    for (size_t i = 0; i < Dim; ++i) x[i] /= at(A, i, i);
  } else static_assert(Part & unit);
}

/// Partially solve a sequence of matrix equations inplace:
/// `x := copy_part<Parts>(A)^-1 * ... * x`.
template<MatPart... Parts, class Mat, mat_multiplier<Mat> Mult>
constexpr void part_solve_inplace(const Mat& A, Mult& x) {
  (part_solve_inplace<Parts>(A, x), ...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
