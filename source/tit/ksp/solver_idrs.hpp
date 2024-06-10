/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/mdvector.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"
#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The IDR(s) (Induced Dimension Reduction) linear operator equation solver.
///
/// References:
/// @verbatim
/// [1] Peter Sonneveld, Martin B. van Gijzen.
///     “IDR(s): A Family of Simple and Fast Algorithms for Solving
///      Large Nonsymmetric Systems of Linear Equations.”
///     SIAM J. Sci. Comput. 31 (2008): 1035-1062.
/// [2] Martin B. van Gijzen, Peter Sonneveld.
///     “Algorithm 913: An Elegant IDR(s) Variant that Efficiently
///      Exploits Biorthogonality Properties.”
///     ACM Trans. Math. Softw. 38 (2011): 5:1-5:19.
/// @endverbatim
template<blas::vector Vector>
class IDRs final : public InnerOuterIterativeSolver<Vector> {
public:

  IDRs() {
    this->NumInnerIterations = 4;
  }

private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init(const Vector& x,
                            const Vector& b,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P)
      -> real_t override {
    const auto s = this->NumInnerIterations;

    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    phi_.resize(s);
    gamma_.resize(s);
    mu_.assign(s, s);

    r_.Assign(x, false);
    v_.Assign(x, false);
    if (P != nullptr) {
      z_.Assign(x, false);
    }

    ps_.resize(s);
    us_.resize(s);
    gs_.resize(s);
    for (Vector& p : ps_) p.Assign(x, false);
    for (Vector& u : us_) u.Assign(x, false);
    for (Vector& g : gs_) g.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒓,
    //   𝒓 ← 𝓟𝒛.
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝜑₀ ← ‖𝒓‖.
    // ----------------------
    A.Residual(r_, b, x);
    if (left_pre) {
      Blas::Swap(z_, r_);
      P->MatVec(r_, z_);
    }
    phi_[0] = Blas::Norm2(r_);

    return phi_[0];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr void inner_init(const Vector& /*x*/,
                            const Vector& /*b*/,
                            const Operator<Vector>& /*A*/,
                            const Preconditioner<Vector>* /*P*/) override {
    const auto s = this->NumInnerIterations;

    // Build shadow space and initialize 𝜑:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝜔 ← 𝜇₀₀ ← 𝟣,
    //   𝒑₀ ← 𝒓/𝜑₀,
    //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑠 - 𝟣 𝗱𝗼:
    //     𝜇ᵢᵢ ← 𝟣, 𝜑ᵢ ← 𝟢,
    //     𝒑ᵢ ← 𝘙𝘢𝘯𝘥𝘰𝘮,
    //     𝗳𝗼𝗿 𝑗 = 𝟢, 𝑖 - 𝟣 𝗱𝗼:
    //       𝜇ᵢⱼ ← 𝟢,
    //       𝒑ᵢ ← 𝒑ᵢ - <𝒑ᵢ⋅𝒑ⱼ>⋅𝒑ⱼ,
    //     𝗲𝗻𝗱 𝗳𝗼𝗿
    //     𝒑ᵢ ← 𝒑ᵢ/‖𝒑ᵢ‖.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑠 - 𝟣 𝗱𝗼:
    //     𝜑ᵢ ← <𝒑ᵢ⋅𝒓>.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    const auto first_iter = this->Iteration == 0;
    if (first_iter) {
      omega_ = mu_[0, 0] = 1.0;
      Blas::Scale(ps_[0], r_, 1.0 / phi_[0]);
      for (size_t i = 1; i < s; ++i) {
        mu_[i, i] = 1.0, phi_[i] = 0.0;
        Blas::RandFill(ps_[i]);
        for (size_t j = 0; j < i; ++j) {
          mu_[i, j] = 0.0;
          Blas::SubAssign(ps_[i], ps_[j], Blas::Dot(ps_[i], ps_[j]));
        }
        Blas::ScaleAssign(ps_[i], 1.0 / Blas::Norm2(ps_[i]));
      }
    } else {
      for (size_t i = 0; i < s; ++i) {
        phi_[i] = Blas::Dot(ps_[i], r_);
      }
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto inner_iter(Vector& x,
                            const Vector& /*b*/,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P)
      -> real_t override {
    const auto s = this->NumInnerIterations;
    const auto k = this->InnerIteration;

    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);
    const auto right_pre = (P != nullptr) && //
                           (this->PreSide == PreconditionerSide::Right);

    // Compute 𝛾:
    // ----------------------
    // 𝛾ₖ:ₛ₋₁ ← (𝜇ₖ:ₛ₋₁,ₖ:ₛ₋₁)⁻¹⋅𝜑ₖ:ₛ₋₁.
    // ----------------------
    for (size_t i = k; i < s; ++i) {
      gamma_[i] = phi_[i];
      for (size_t j = k; j < i; ++j) {
        gamma_[i] -= mu_[i, j] * gamma_[j];
      }
      gamma_[i] /= mu_[i, i];
    }

    // Compute the new 𝒈ₖ and 𝒖ₖ vectors:
    // ----------------------
    // 𝒗 ← 𝒓 - 𝛾ₖ⋅𝒈ₖ,
    // 𝗳𝗼𝗿 𝑖 = 𝑘 + 𝟣, 𝑠 - 𝟣 𝗱𝗼:
    //   𝒗 ← 𝒗 - 𝛾ᵢ⋅𝒈ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒗,
    //   𝒗 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒖ₖ ← 𝜔⋅𝒗 + 𝛾ₖ⋅𝒖ₖ,
    // 𝗳𝗼𝗿 𝑖 = 𝑘 + 𝟣, 𝑠 - 𝟣 𝗱𝗼:
    //   𝒖ₖ ← 𝒖ₖ + 𝛾ᵢ⋅𝒖ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒈ₖ ← 𝓟(𝒛 ← 𝓐𝒖ₖ).
    // 𝗲𝗹𝘀𝗲:
    //   𝒈ₖ ← 𝓐𝒖ₖ.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    Blas::Sub(v_, r_, gs_[k], gamma_[k]);
    for (size_t i = k + 1; i < s; ++i) {
      Blas::SubAssign(v_, gs_[i], gamma_[i]);
    }
    if (right_pre) {
      Blas::Swap(z_, v_);
      P->MatVec(v_, z_);
    }
    Blas::Add(us_[k], us_[k], gamma_[k], v_, omega_);
    for (size_t i = k + 1; i < s; ++i) {
      Blas::AddAssign(us_[k], us_[i], gamma_[i]);
    }
    if (left_pre) {
      P->MatVec(gs_[k], z_, A, us_[k]);
    } else {
      A.MatVec(gs_[k], us_[k]);
    }

    // Biorthogonalize the new vectors 𝒈ₖ and 𝒖ₖ:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 - 𝟣 𝗱𝗼:
    //   𝛼 ← <𝒑ᵢ⋅𝒈ₖ>/𝜇ᵢᵢ,
    //   𝒖ₖ ← 𝒖ₖ - 𝛼⋅𝒖ᵢ,
    //   𝒈ₖ ← 𝒈ₖ - 𝛼⋅𝒈ᵢ.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    for (size_t i = 0; i < k; ++i) {
      const auto alpha = safe_divide(Blas::Dot(ps_[i], gs_[k]), mu_[i, i]);
      Blas::SubAssign(us_[k], us_[i], alpha);
      Blas::SubAssign(gs_[k], gs_[i], alpha);
    }

    // Compute the new column of 𝜇:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝑘, 𝑠 - 𝟣 𝗱𝗼:
    //   𝜇ᵢₖ ← <𝒑ᵢ⋅𝒈ₖ>.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    for (size_t i = k; i < s; ++i) {
      mu_[i, k] = Blas::Dot(ps_[i], gs_[k]);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝛽 ← 𝜑ₖ/𝜇ₖₖ,
    // 𝒙 ← 𝒙 + 𝛽⋅𝒖ₖ,
    // 𝒓 ← 𝒓 - 𝛽⋅𝒈ₖ.
    // ----------------------
    const auto beta = safe_divide(phi_[k], mu_[k, k]);
    Blas::AddAssign(x, us_[k], beta);
    Blas::SubAssign(r_, gs_[k], beta);

    // Update 𝜑:
    // ----------------------
    // 𝜑ₖ₊₁:ₛ₋₁ ← 𝜑ₖ₊₁:ₛ₋₁ - 𝛽⋅𝜇ₖ₊₁:ₛ₋₁,ₖ.
    // ----------------------
    for (size_t i = k + 1; i < s; ++i) {
      phi_[i] -= beta * mu_[i, k];
    }

    if (k == s - 1) {
      // Enter the next 𝓖 subspace:
      // ----------------------
      // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
      //   𝒗 ← 𝓟(𝒛 ← 𝓐𝒓),
      // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
      //   𝒗 ← 𝓐(𝒛 ← 𝓟𝒓),
      // 𝗲𝗹𝘀𝗲:
      //   𝒗 ← 𝓐𝒓,
      // 𝗲𝗻𝗱 𝗶𝗳
      // 𝜔 ← <𝒗⋅𝒓>/<𝒗⋅𝒗>,
      // 𝒙 ← 𝒙 + 𝜔⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒓),
      // 𝒓 ← 𝒓 - 𝜔⋅𝒗.
      // ----------------------
      if (left_pre) {
        P->MatVec(v_, z_, A, r_);
      } else if (right_pre) {
        A.MatVec(v_, z_, *P, r_);
      } else {
        A.MatVec(v_, r_);
      }
      omega_ = safe_divide(Blas::Dot(v_, r_), Blas::Dot(v_, v_));
      Blas::AddAssign(x, right_pre ? z_ : r_, omega_);
      Blas::SubAssign(r_, v_, omega_);
    }

    return Blas::Norm2(r_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t omega_{};
  std::vector<real_t> phi_, gamma_;
  Mdvector<real_t, 2> mu_;
  Vector r_, v_, z_;
  std::vector<Vector> ps_, us_, gs_;

}; // class IDRs

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
