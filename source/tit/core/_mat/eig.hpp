/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/mat.hpp"
#pragma once

#include <expected>
#include <type_traits>
#include <utility>

#include "tit/core/_mat/mat.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Matrix eigenvectors and eigenvalues.
template<class Num, size_t Dim>
struct MatEig {
  Mat<Num, Dim> vecs; ///< Eigenvectors of a matrix.
  Vec<Num, Dim> vals; ///< Eigenvalues of a matrix.
};

/// Matrix eigensolver error type.
enum class MatEigError : uint8_t {
  no_error,      ///< No error.
  not_converged, ///< The eigensolver failed to converge.
};

/// Matrix eigenvalue problem result.
template<class Num, size_t Dim>
using MatEigResult = std::expected<MatEig<Num, Dim>, MatEigError>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the eigenvectors and eigenvalues of a symmetric matrix using
/// Jacobi eigenvalue algorithm.
///
/// The result of this operation is a pair of a pair a vector `d` that
/// contains eigenvalues of an input matrix `A` and a matrix `V`, whose
/// rows contain the corresponding eigenvectors. Thus, `V * A == diag(d) * V`
/// and `A * V[k] == d[k] * V[k]`.
///
/// Only the lower-triangular part of the input matrix is accessed.
template<class Num, size_t Dim>
constexpr auto jacobi(Mat<Num, Dim> A,
                      std::type_identity_t<Num> eps = tiny_number_v<Num>,
                      size_t max_iter = Dim * 32) -> MatEigResult<Num, Dim> {
  auto V = eye(A);
  if constexpr (Dim == 1) {
    return MatEig{std::move(V), Vec{std::move(A[0, 0])}};
  }

  for (size_t iter = 0; iter < max_iter; ++iter) {
    // Find maximum off-diagonal element.
    size_t p = 1;
    size_t q = 0;
    for (size_t i = 2; i < Dim; ++i) {
      for (size_t j = 0; j < i; ++j) {
        if (abs(A[i, j]) > abs(A[p, q])) {
          p = i;
          q = j;
        }
      }
    }

    // If the maximum off-diagonal element is below the threshold, then the
    // matrix is considered diagonal, and the algorithm has converged.
    if (abs(A[p, q]) <= eps) {
      return MatEig{std::move(V), diag(std::move(A))};
    }

    // Compute the rotation angle.
    const auto theta = Num{0.5} * atan2(Num{2.0} * A[p, q], A[q, q] - A[p, p]);
    const auto c = cos(theta);
    const auto s = sin(theta);

    // Update the matrix.
    for (size_t i = 0; i < Dim; ++i) {
      if (i == p || i == q) continue;
      const auto Api = A[p, i];
      const auto Aqi = A[q, i];
      A[p, i] = A[i, p] = c * Api - s * Aqi;
      A[q, i] = A[i, q] = s * Api + c * Aqi;
    }
    const auto App = A[p, p];
    const auto Apq = A[p, q];
    const auto Aqq = A[q, q];
    A[p, p] = c * (c * App - s * Apq) - s * (c * Apq - s * Aqq);
    A[q, q] = s * (s * App + c * Apq) + c * (s * Apq + c * Aqq);
    A[p, q] = A[q, p] = {};

    // Update the eigenvectors.
    for (size_t i = 0; i < Dim; ++i) {
      const auto Vpi = V[p, i];
      const auto Vqi = V[q, i];
      V[p, i] = c * Vpi - s * Vqi;
      V[q, i] = s * Vpi + c * Vqi;
    }
  }

  return std::unexpected{MatEigError::not_converged};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
