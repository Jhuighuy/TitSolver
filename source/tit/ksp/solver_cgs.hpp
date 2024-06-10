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

/// The CGS (Conjugate Gradients Squared) linear operator equation solver.
///
/// CGS, like the other BiCG type solvers, requires two operator multiplications
/// per iteration.
///
/// References:
/// @verbatim
/// [1] Sonneveld, Peter.
///     “CGS, A Fast Lanczos-Type Solver for Nonsymmetric Linear systems.”
///     SIAM J. Sci. Stat. Comput., 10:36-52, 1989.
/// @endverbatim
template<blas::vector Vector>
class CGS final : public IterativeSolver<Vector> {
private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t override {
    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    p_.Assign(x, false);
    q_.Assign(x, false);
    r_.Assign(x, false);
    r_tilde_.Assign(x, false);
    u_.Assign(x, false);
    v_.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒖 ← 𝒓,
    //   𝒓 ← 𝓟𝒖,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓,
    // 𝜌 ← <𝒓̃⋅𝒓>.
    // ----------------------
    A.Residual(r_, b, x);
    if (left_pre) {
      Blas::Swap(u_, r_);
      P->MatVec(r_, u_);
    }
    Blas::Set(r_tilde_, r_);
    rho_ = Blas::Dot(r_tilde_, r_);

    return sqrt(rho_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto iter(Vector& x,
                      const Vector& /*b*/,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t override {
    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);
    const auto right_pre = (P != nullptr) && //
                           (this->PreSide == PreconditionerSide::Right);

    // Continue the iterations:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒖 ← 𝒓,
    //   𝒑 ← 𝒖.
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓>,
    //   𝛽 ← 𝜌/𝜌̅,
    //   𝒖 ← 𝒓 + 𝛽⋅𝒒,
    //   𝒑 ← 𝒒 + 𝛽⋅𝒑,
    //   𝒑 ← 𝒖 + 𝛽⋅𝒑.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    const auto first_iter = this->Iteration == 0;
    if (first_iter) {
      Blas::Set(u_, r_);
      Blas::Set(p_, u_);
    } else {
      const auto rho_bar = rho_;
      rho_ = Blas::Dot(r_tilde_, r_);
      const auto beta = safe_divide(rho_, rho_bar);
      Blas::Add(u_, r_, q_, beta);
      Blas::Add(p_, q_, p_, beta);
      Blas::Add(p_, u_, p_, beta);
    }

    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓟(𝒒 ← 𝓐𝒑),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒒 ← 𝓟𝒑),
    // 𝗲𝗹𝘀𝗲:
    //   𝒗 ← 𝓐𝒑,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝒒 ← 𝒖 - 𝛼⋅𝒗,
    // 𝒗 ← 𝒖 + 𝒒.
    // ----------------------
    if (left_pre) {
      P->MatVec(v_, q_, A, p_);
    } else if (right_pre) {
      A.MatVec(v_, q_, *P, p_);
    } else {
      A.MatVec(v_, p_);
    }
    const auto alpha = safe_divide(rho_, Blas::Dot(r_tilde_, v_));
    Blas::Sub(q_, u_, v_, alpha);
    Blas::Add(v_, u_, q_);

    // Update the solution and the residual:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒙 ← 𝒙 + 𝛼⋅𝒗,
    //   𝒗 ← 𝓟(𝒖 ← 𝓐𝒗),
    //   𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒖 ← 𝓟𝒗),
    //   𝒙 ← 𝒙 + 𝛼⋅𝒖,
    //   𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // 𝗲𝗹𝘀𝗲:
    //   𝒖 ← 𝓐𝒗,
    //   𝒙 ← 𝒙 + 𝛼⋅𝒗,
    //   𝒓 ← 𝒓 - 𝛼⋅𝒖.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (left_pre) {
      Blas::AddAssign(x, v_, alpha);
      P->MatVec(v_, u_, A, v_);
      Blas::SubAssign(r_, v_, alpha);
    } else if (right_pre) {
      A.MatVec(v_, u_, *P, v_);
      Blas::AddAssign(x, u_, alpha);
      Blas::SubAssign(r_, v_, alpha);
    } else {
      A.MatVec(u_, v_);
      Blas::AddAssign(x, v_, alpha);
      Blas::SubAssign(r_, u_, alpha);
    }

    return Blas::Norm2(r_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t rho_;
  Vector p_, q_, r_, r_tilde_, u_, v_;

}; // class CGS

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
