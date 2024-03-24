/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/mat/mat.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/vec.hpp"

namespace tit {

/// Below are some matrix-related utilities that are mostly used as common
/// parts of other matrix algorithms.
/// @{

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
};

template<>
inline constexpr bool is_flags_enum_v<MatPart> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Copy the matrix part.
template<MatPart Part, class Num, size_t Dim>
constexpr auto copy_part(Mat<Num, Dim> const& A) -> Mat<Num, Dim> {
  Mat<Num, Dim> R{};
  for (size_t i = 0; i < Dim; ++i) {
    if constexpr (Part & MatPart::lower) {
      for (size_t j = 0; j < i; ++j) R[i, j] = A[i, j];
    }
    if constexpr (Part & MatPart::diag) R[i, i] = A[i, i];
    else if constexpr (Part & MatPart::unit) R[i, i] = Num{1.0};
    if constexpr (Part & MatPart::upper) {
      for (size_t j = i + 1; j < Dim; ++j) R[i, j] = A[i, j];
    }
  }
  return R;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partially solve matrix equation inplace:
/// `x := copy_part<Part>(A)^-1 * x`.
template<MatPart Part, class Num, size_t Dim, class Arg>
  requires (std::same_as<Arg, Vec<Num, Dim>> ||
            std::same_as<Arg, Mat<Num, Dim>>)
constexpr void solve_part(Mat<Num, Dim> const& A, Arg& x) {
  static_assert(Part & (MatPart::diag | MatPart::unit),
                "Diagonal part or a unit diagonal must be specified!");
  if constexpr (Part == MatPart::diag) {
    for (size_t i = 0; i < Dim; ++i) x[i] /= A[i, i];
  } else if constexpr (Part & MatPart::lower) {
    static_assert(!(Part & MatPart::upper),
                  "Cannot solve for both lower and upper parts!");
    for (size_t i = 0; i < Dim; ++i) {
      for (size_t j = 0; j < i; ++j) x[i] -= A[i, j] * x[j];
      if constexpr (Part & MatPart::diag) x[i] /= A[i, i];
    }
  } else if constexpr (Part & MatPart::upper) {
    for (ssize_t i = Dim - 1; i >= 0; --i) {
      for (size_t j = i + 1; j < Dim; ++j) x[i] -= A[i, j] * x[j];
      if constexpr (Part & MatPart::diag) x[i] /= A[i, i];
    }
  } else static_assert(false);
}

/// Partially solve transposed matrix equation inplace:
/// `x := copy_part<Part>(A)^-T * x`.
template<MatPart Part, class Num, size_t Dim, class Arg>
  requires (std::same_as<Arg, Vec<Num, Dim>> ||
            std::same_as<Arg, Mat<Num, Dim>>)
constexpr void solve_part_transposed(Mat<Num, Dim> const& A, Arg& x) {
  static_assert(Part & (MatPart::diag | MatPart::unit),
                "Diagonal part or a unit diagonal must be specified!");
  static_assert(Part & (MatPart::lower | MatPart::upper),
                "Lower or upper triangular part must be specified!");
  if constexpr (Part & MatPart::lower) {
    static_assert(!(Part & MatPart::upper),
                  "Cannot solve for both lower and upper parts!");
    for (ssize_t i = Dim - 1; i >= 0; --i) {
      for (size_t j = i + 1; j < Dim; ++j) x[i] -= A[j, i] * x[j];
      if constexpr (Part & MatPart::diag) x[i] /= A[i, i];
    }
  } else if constexpr (Part & MatPart::upper) {
    for (size_t i = 0; i < Dim; ++i) {
      for (size_t j = 0; j < i; ++j) x[i] -= A[j, i] * x[j];
      if constexpr (Part & MatPart::diag) x[i] /= A[i, i];
    }
  } else static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Product of the diagonal elements.
template<class Num, size_t Dim>
constexpr auto diag_prod(Mat<Num, Dim> const& A) -> Num {
  auto r = A[0, 0];
  for (size_t i = 1; i < Dim; ++i) r *= A[i, i];
  return r;
}

/// Determinant of a matrix (for matrix size up to 3x3).
template<class Num, size_t Dim>
  requires in_range_v<Dim, 1, 3>
constexpr auto det(Mat<Num, Dim> const& A) -> Num {
  if constexpr (Dim == 1) {
    return A[0, 0];
  } else if constexpr (Dim == 2) {
    return A[0, 0] * A[1, 1] - A[0, 1] * A[1, 0];
  } else if constexpr (Dim == 3) {
    return A[0, 0] * (A[1, 1] * A[2, 2] - A[1, 2] * A[2, 1]) +
           A[0, 1] * (A[1, 2] * A[2, 0] - A[1, 0] * A[2, 2]) +
           A[0, 2] * (A[1, 0] * A[2, 1] - A[1, 1] * A[2, 0]);
  } else static_assert(false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @}

} // namespace tit
