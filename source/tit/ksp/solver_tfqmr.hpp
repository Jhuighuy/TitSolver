/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"
#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Base class for TFQMR and TFQMR1.
template<blas::vector Vector, bool L1>
class BaseTFQMR : public IterativeSolver<Vector> {
protected:

  constexpr BaseTFQMR() = default;

private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t override {
    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    d_.Assign(x, false);
    r_tilde_.Assign(x, false);
    u_.Assign(x, false);
    v_.Assign(x, false);
    y_.Assign(x, false);
    s_.Assign(x, false);
    if (P != nullptr) z_.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝗶𝗳 𝘓₁:
    //   𝒅 ← 𝒙,
    // 𝗲𝗹𝘀𝗲:
    //   𝒅 ← {𝟢}ᵀ,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒚 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒚,
    //   𝒚 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒖 ← 𝒚,
    // 𝒓̃ ← 𝒖,
    // 𝜌 ← <𝒓̃⋅𝒓>, 𝜏 ← 𝜌¹ᐟ².
    // ----------------------
    if constexpr (L1) {
      Blas::Set(d_, x);
    } else {
      Blas::Fill(d_, 0.0);
    }
    A.Residual(y_, b, x);
    if (left_pre) {
      std::swap(z_, y_);
      P->MatVec(y_, z_);
    }
    Blas::Set(u_, y_);
    Blas::Set(r_tilde_, u_);
    rho_ = Blas::Dot(r_tilde_, u_);
    tau_ = sqrt(rho_);

    return tau_;
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
    //   𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓟(𝒛 ← 𝓐𝒚),
    //   𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓐(𝒛 ← 𝓟𝒚),
    //   𝗲𝗹𝘀𝗲:
    //     𝒔 ← 𝓐𝒚.
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝒗 ← 𝒔,
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒖>,
    //   𝛽 ← 𝜌/𝜌̅,
    //   𝒗 ← 𝒔 + 𝛽⋅𝒗,
    //   𝒚 ← 𝒖 + 𝛽⋅𝒚,
    //   𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓟(𝒛 ← 𝓐𝒚),
    //   𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓐(𝒛 ← 𝓟𝒚),
    //   𝗲𝗹𝘀𝗲:
    //     𝒔 ← 𝓐𝒚,
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝒗 ← 𝒔 + 𝛽⋅𝒗.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    const auto first_iter = this->Iteration == 0;
    if (first_iter) {
      if (left_pre) {
        P->MatVec(s_, z_, A, y_);
      } else if (right_pre) {
        A.MatVec(s_, z_, *P, y_);
      } else {
        A.MatVec(s_, y_);
      }
      Blas::Set(v_, s_);
    } else {
      const auto rho_bar = rho_;
      rho_ = Blas::Dot(r_tilde_, u_);
      const auto beta = safe_divide(rho_, rho_bar);
      Blas::Add(v_, s_, v_, beta);
      Blas::Add(y_, u_, y_, beta);
      if (left_pre) {
        P->MatVec(s_, z_, A, y_);
      } else if (right_pre) {
        A.MatVec(s_, z_, *P, y_);
      } else {
        A.MatVec(s_, y_);
      }
      Blas::Add(v_, s_, v_, beta);
    }

    // Update the solution:
    // ----------------------
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝗳𝗼𝗿 𝑚 = 𝟢, 𝟣 𝗱𝗼:
    //   𝒖 ← 𝒖 - 𝛼⋅𝒔,
    //   𝒅 ← 𝒅 + 𝛼⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒚),
    //   𝜔 ← ‖𝒖‖,
    //   𝗶𝗳 𝘓₁:
    //     𝗶𝗳 𝜔 < 𝜏:
    //       𝜏 ← 𝜔, 𝒙 ← 𝒅,
    //     𝗲𝗻𝗱 𝗶𝗳
    //   𝗲𝗹𝘀𝗲:
    //     𝑐𝑠, 𝑠𝑛 ← 𝘚𝘺𝘮𝘖𝘳𝘵𝘩𝘰(𝜏, 𝜔),
    //     𝜏 ← 𝑐𝑠⋅𝜔,
    //     𝒙 ← 𝒙 + 𝑐𝑠²⋅𝒅,
    //     𝒅 ← 𝑠𝑛²⋅𝒅,
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝗶𝗳 𝑚 = 𝟢:
    //     𝒚 ← 𝒚 - 𝛼⋅𝒗,
    //     𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //       𝒔 ← 𝓟(𝒛 ← 𝓐𝒚).
    //     𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //       𝒔 ← 𝓐(𝒛 ← 𝓟𝒚).
    //     𝗲𝗹𝘀𝗲:
    //       𝒔 ← 𝓐𝒚.
    //     𝗲𝗻𝗱 𝗶𝗳
    //   𝗲𝗻𝗱 𝗶𝗳
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    const auto alpha = safe_divide(rho_, Blas::Dot(r_tilde_, v_));
    for (size_t m = 0; m <= 1; ++m) {
      Blas::SubAssign(u_, s_, alpha);
      Blas::AddAssign(d_, right_pre ? z_ : y_, alpha);
      const auto omega = Blas::Norm2(u_);
      if constexpr (L1) {
        if (omega < tau_) {
          tau_ = omega, Blas::Set(x, d_);
        }
      } else {
        const auto [cs, sn, rr] = sym_ortho(tau_, omega);
        tau_ = omega * cs;
        Blas::AddAssign(x, d_, pow2(cs));
        Blas::ScaleAssign(d_, pow2(sn));
      }
      if (m == 0) {
        Blas::SubAssign(y_, v_, alpha);
        if (left_pre) {
          P->MatVec(s_, z_, A, y_);
        } else if (right_pre) {
          A.MatVec(s_, z_, *P, y_);
        } else {
          A.MatVec(s_, y_);
        }
      }
    }

    // Compute the residual norm
    // (or it's upper bound estimate in the ℒ₂ case):
    // ----------------------
    // 𝜏̃ ← 𝜏,
    // 𝗶𝗳 𝗻𝗼𝘁 𝘓₁:
    //   𝜏̃ ← 𝜏⋅(𝟤𝑘 + 𝟥)¹ᐟ².
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    auto tau_tilde = tau_;
    if constexpr (!L1) {
      const auto k = this->Iteration;
      tau_tilde *= sqrt(2.0 * k + 3.0);
    }

    return tau_tilde;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t rho_{}, tau_{};
  Vector d_, r_tilde_, u_, v_, y_, s_, z_;

}; // class BaseTFQMR

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The TFQMR (Transpose-Free Quasi-Minimal Residual) linear operator equation
/// solver.
///
/// TFQMR, like the other BiCG type methods, normally requires two
/// operator-vector products per iteration. But, unlike the other BiCG type
/// methods, TFQMR does not implicitly contain the residual norm estimate, only
/// the rough upper bound is avariable, so at the latter iterations an extra
/// operator-vector product per iteration may be required for the explicit
/// residual estimation.
///
/// References:
/// @verbatim
/// [1] Freund, Roland W.
///     “A Transpose-Free Quasi-Minimal Residual Algorithm
///      for Non-Hermitian Linear Systems.”
///     SIAM J. Sci. Comput. 14 (1993): 470-482.
/// [2] Freund, Roland W.
///     “Transpose-Free Quasi-Minimal Residual Methods
///      for Non-Hermitian Linear Systems.” (1994).
/// @endverbatim
template<blas::vector Vector>
class TFQMR final : public impl::BaseTFQMR<Vector, /*L1=*/false> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The TFQMR1 (Transpose-Free 1-norm Quasi-Minimal Residual) linear operator
/// equation solver.
///
/// TFQMR1, like the other BiCG type solvers, requires two operator-vector
/// products per iteration. Unlike TFQMR, TFQMR1 implicitly contains the
/// residual norm estimate, so no extra operator-vector products are required.
///
/// References:
/// @verbatim
/// [1] H.M Bücker,
///     “A Transpose-Free 1-norm Quasi-Minimal Residual Algorithm
///      for Non-Hermitian Linear Systems.“, FZJ-ZAM-IB-9706.
/// @endverbatim
template<blas::vector Vector>
class TFQMR1 final : public impl::BaseTFQMR<Vector, /*L1=*/true> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
