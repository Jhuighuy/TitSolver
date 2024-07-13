/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"
#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The CG (Conjugate Gradients) linear self-adjoint definite operator equation
/// solver.
///
/// References:
/// @verbatim
/// [1] Hestenes, Magnus R. and Eduard Stiefel.
///     “Methods of conjugate gradients for solving linear systems.”
///     Journal of research of the National
///     Bureau of Standards 49 (1952): 409-435.
/// @endverbatim
template<blas::vector Vector>
class CG final : public IterativeSolver<Vector> {
private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t {
    p_.Assign(x, false);
    r_.Assign(x, false);
    z_.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙.
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒛 ← 𝓟𝒓,
    //   𝒑 ← 𝒛,
    //   𝛾 ← <𝒓⋅𝒛>,
    // 𝗲𝗹𝘀𝗲:
    //   𝒑 ← 𝒓,
    //   𝛾 ← <𝒓⋅𝒓>.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    A.Residual(r_, b, x);
    if (P != nullptr) {
      P->MatVec(z_, r_);
      Blas::Set(p_, z_);
      gamma_ = Blas::Dot(r_, z_);
    } else {
      Blas::Set(p_, r_);
      gamma_ = Blas::Dot(r_, r_);
    }

    return (P != nullptr) ? Blas::Norm2(r_) : sqrt(gamma_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto iter(Vector& x,
                      const Vector& /*b*/,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t {
    // Iterate:
    // ----------------------
    // 𝒛 ← 𝓐𝒑,
    // 𝛾̅ ← 𝛾,
    // 𝛼 ← 𝛾/<𝒑⋅𝒛>,
    // 𝒙 ← 𝒙 + 𝛼⋅𝒑,
    // 𝒓 ← 𝒓 - 𝛼⋅𝒛.
    // ----------------------
    A.MatVec(z_, p_);
    const auto gamma_bar = gamma_;
    const auto alpha = safe_divide(gamma_, Blas::Dot(p_, z_));
    Blas::AddAssign(x, p_, alpha);
    Blas::SubAssign(r_, z_, alpha);

    // ----------------------
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒛 ← 𝓟𝒓,
    //   𝛾 ← <𝒓⋅𝒛>,
    // 𝗲𝗹𝘀𝗲:
    //   𝛾 ← <𝒓⋅𝒓>.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (P != nullptr) {
      P->MatVec(z_, r_);
      gamma_ = Blas::Dot(r_, z_);
    } else {
      gamma_ = Blas::Dot(r_, r_);
    }

    // ----------------------
    // 𝛽 ← 𝛾/𝛾̅,
    // 𝒑 ← (𝓟 ≠ 𝗻𝗼𝗻𝗲 ? 𝒛 : 𝒓) + 𝛽⋅𝒑.
    // ----------------------
    const auto beta = safe_divide(gamma_, gamma_bar);
    Blas::Add(p_, P != nullptr ? z_ : r_, p_, beta);

    return (P != nullptr) ? Blas::Norm2(r_) : sqrt(gamma_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t gamma_;
  Vector p_, r_, z_;

  friend class IterativeSolver<Vector>;

}; // class CG

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
