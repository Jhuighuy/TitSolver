/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>
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

/// The BiCGStab (Biconjugate Gradients Stabilized) linear operator equation
/// solver.
///
/// BiCGStab, like the other BiCG type solvers, requires two operator
/// multiplications per iteration.
///
/// References:
/// @verbatim
/// [1] Henk A. van der Vorst.
///     “Bi-CGSTAB: A Fast and Smoothly Converging Variant of Bi-CG
///      for the Solution of Nonsymmetric Linear Systems.”
///     SIAM J. Sci. Comput. 13 (1992): 631-644.
/// @endverbatim
template<blas::vector Vector>
class BiCGStab final : public IterativeSolver<Vector> {
private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t {
    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    p_.Assign(x, false);
    r_.Assign(x, false);
    r_tilde_.Assign(x, false);
    t_.Assign(x, false);
    v_.Assign(x, false);
    if (P != nullptr) z_.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒓,
    //   𝒓 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓,
    // 𝜌 ← <𝒓̃⋅𝒓>.
    // ----------------------
    A.Residual(r_, b, x);
    if (left_pre) {
      std::swap(z_, r_);
      P->MatVec(r_, z_);
    }
    Blas::Set(r_tilde_, r_);
    rho_ = Blas::Dot(r_tilde_, r_);

    return sqrt(rho_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto iter(Vector& x,
                      const Vector& /*b*/,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* P) -> real_t {
    const auto left_pre = (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);
    const auto right_pre = (P != nullptr) && //
                           (this->PreSide == PreconditionerSide::Right);

    // Continue the iterations:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒑 ← 𝒓.
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓>,
    //   𝛽 ← (𝜌/𝜌̅)⋅(𝛼/𝜔),
    //   𝒑 ← 𝒑 - 𝜔⋅𝒗,
    //   𝒑 ← 𝒓 + 𝛽⋅𝒑.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    const auto first_iter = this->Iteration == 0;
    if (first_iter) {
      Blas::Set(p_, r_);
    } else {
      const auto rho_bar = rho_;
      rho_ = Blas::Dot(r_tilde_, r_);
      const auto beta = safe_divide(alpha_ * rho_, omega_ * rho_bar);
      Blas::SubAssign(p_, v_, omega_);
      Blas::Add(p_, r_, p_, beta);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓟(𝒛 ← 𝓐𝒑),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒛 ← 𝓟𝒑),
    // 𝗲𝗹𝘀𝗲:
    //   𝒗 ← 𝓐𝒑,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝒙 ← 𝒙 + 𝛼⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒑),
    // 𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // ----------------------
    if (left_pre) P->MatVec(v_, z_, A, p_);
    else if (right_pre) {
      A.MatVec(v_, z_, *P, p_);
    } else {
      A.MatVec(v_, p_);
    }
    alpha_ = safe_divide(rho_, Blas::Dot(r_tilde_, v_));
    Blas::AddAssign(x, right_pre ? z_ : p_, alpha_);
    Blas::SubAssign(r_, v_, alpha_);

    // Update the solution and the residual again:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒕 ← 𝓟(𝒛 ← 𝓐𝒓),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒕 ← 𝓐(𝒛 ← 𝓟𝒓),
    // 𝗲𝗹𝘀𝗲:
    //   𝒕 ← 𝓐𝒓,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝜔 ← <𝒕⋅𝒓>/<𝒕⋅𝒕>,
    // 𝒙 ← 𝒙 + 𝜔⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒓),
    // 𝒓 ← 𝒓 - 𝜔⋅𝒕.
    // ----------------------
    if (left_pre) {
      P->MatVec(t_, z_, A, r_);
    } else if (right_pre) {
      A.MatVec(t_, z_, *P, r_);
    } else {
      A.MatVec(t_, r_);
    }
    omega_ = safe_divide(Blas::Dot(t_, r_), Blas::Dot(t_, t_));
    Blas::AddAssign(x, right_pre ? z_ : r_, omega_);
    Blas::SubAssign(r_, t_, omega_);

    return Blas::Norm2(r_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t alpha_, rho_, omega_;
  Vector p_, r_, r_tilde_, t_, v_, z_;

  friend class IterativeSolver<Vector>;

}; // class BiCGStab

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The BiCGStab(l) (Biconjugate Gradients Stabilized) linear operator equation
/// solver.
///
/// BiCGStab(l), like the other BiCG type solvers, requires two operator
/// multiplications per iteration.
///
/// References:
/// @verbatim
/// [1] Gerard L. G. Sleijpen and Diederik R. Fokkema.
///     “BiCGStab(l) for Linear Equations involving
///      Unsymmetric Matrices with Complex Spectrum.”
///     Electronic Transactions on Numerical Analysis 1 (1993): 11-32.
/// @endverbatim
template<blas::vector Vector>
class BiCGStabL final : public InnerOuterIterativeSolver<Vector> {
public:

  BiCGStabL() {
    this->NumInnerIterations = 3;
  }

private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init(const Vector& x,
                            const Vector& b,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P) -> real_t {
    const auto l = this->NumInnerIterations;

    gamma_.resize(l + 1);
    gamma_bar_.resize(l + 1);
    gamma_bbar_.resize(l + 1);
    sigma_.resize(l + 1);
    tau_.assign(l + 1, l + 1);

    r_tilde_.Assign(x, false);
    if (P != nullptr) z_.Assign(x, false);

    rs_.resize(l + 1);
    us_.resize(l + 1);
    for (Vector& r : rs_) r.Assign(x, false);
    for (Vector& u : us_) u.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒖₀ ← {𝟢}ᵀ,
    // 𝒓₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒛 ← 𝒓₀,
    //   𝒓₀ ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓₀,
    // 𝜌 ← <𝒓̃⋅𝒓₀>.
    // ----------------------
    Blas::Fill(us_[0], 0.0);
    A.Residual(rs_[0], b, x);
    if (P != nullptr) {
      std::swap(z_, rs_[0]);
      P->MatVec(rs_[0], z_);
    }
    Blas::Set(r_tilde_, rs_[0]);
    rho_ = Blas::Dot(r_tilde_, rs_[0]);

    return sqrt(rho_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto inner_iter(Vector& x,
                            const Vector& /*b*/,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P) -> real_t {
    const auto l = this->NumInnerIterations;
    const auto j = this->InnerIteration;

    // BiCG part:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒖₀ ← 𝒓₀,
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓ⱼ>,
    //   𝛽 ← 𝛼⋅𝜌/𝜌̅,
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑗 𝗱𝗼:
    //     𝒖ᵢ ← 𝒓ᵢ - 𝛽⋅𝒖ᵢ,
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒖ⱼ₊₁ ← 𝓟(𝒛 ← 𝓐𝒖ⱼ),
    // 𝗲𝗹𝘀𝗲:
    //   𝒖ⱼ₊₁ ← 𝓐𝒖ⱼ,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒖ⱼ₊₁>,
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑗 𝗱𝗼:
    //   𝒓ᵢ ← 𝒓ᵢ - 𝛼⋅𝒖ᵢ₊₁.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    const auto first_iter = this->Iteration == 0;
    if (first_iter) {
      Blas::Set(us_[0], rs_[0]);
    } else {
      const auto rho_bar = rho_;
      rho_ = Blas::Dot(r_tilde_, rs_[j]);
      const auto beta = safe_divide(alpha_ * rho_, rho_bar);
      for (size_t i = 0; i <= j; ++i) {
        Blas::Sub(us_[i], rs_[i], us_[i], beta);
      }
    }
    if (P != nullptr) {
      P->MatVec(us_[j + 1], z_, A, us_[j]);
    } else {
      A.MatVec(us_[j + 1], us_[j]);
    }
    alpha_ = safe_divide(rho_, Blas::Dot(r_tilde_, us_[j + 1]));
    for (size_t i = 0; i <= j; ++i) {
      Blas::SubAssign(rs_[i], us_[i + 1], alpha_);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝒙 ← 𝒙 + 𝛼⋅𝒖₀,
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒓ⱼ₊₁ ← 𝓟(𝒛 ← 𝓐𝒓ⱼ).
    // 𝗲𝗹𝘀𝗲:
    //   𝒓ⱼ₊₁ ← 𝓐𝒓ⱼ.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    Blas::AddAssign(x, us_[0], alpha_);
    if (P != nullptr) {
      P->MatVec(rs_[j + 1], z_, A, rs_[j]);
    } else {
      A.MatVec(rs_[j + 1], rs_[j]);
    }

    if (j == l - 1) {
      // Minimal residual part:
      // ----------------------
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 𝗱𝗼:
      //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑗 - 𝟣 𝗱𝗼:
      //     𝜏ᵢⱼ ← <𝒓ᵢ⋅𝒓ⱼ>/𝜎ᵢ,
      //     𝒓ⱼ ← 𝒓ⱼ - 𝜏ᵢⱼ⋅𝒓ᵢ,
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      //   𝜎ⱼ ← <𝒓ⱼ⋅𝒓ⱼ>,
      //   𝛾̅ⱼ ← <𝒓₀⋅𝒓ⱼ>/𝜎ⱼ,
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      for (size_t k = 1; k <= l; ++k) {
        for (size_t i = 1; i < k; ++i) {
          tau_[i, k] = safe_divide(Blas::Dot(rs_[i], rs_[k]), sigma_[i]);
          Blas::SubAssign(rs_[k], rs_[i], tau_[i, k]);
        }
        sigma_[k] = Blas::Dot(rs_[k], rs_[k]);
        gamma_bar_[k] = safe_divide(Blas::Dot(rs_[0], rs_[k]), sigma_[k]);
      }

      // ----------------------
      // 𝜔 ← 𝛾ₗ ← 𝛾̅ₗ, 𝜌 ← -𝜔⋅𝜌,
      // 𝗳𝗼𝗿 𝑗 = 𝑙 - 𝟣, 𝟣, -𝟣 𝗱𝗼:
      //   𝛾ⱼ ← 𝛾̅ⱼ,
      //   𝗳𝗼𝗿 𝑖 = 𝑗 + 𝟣, 𝑙 𝗱𝗼:
      //     𝛾ⱼ ← 𝛾ⱼ - 𝜏ⱼᵢ⋅𝛾ᵢ,
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //   𝛾̿ⱼ ← 𝛾ⱼ₊₁,
      //   𝗳𝗼𝗿 𝑖 = 𝑗 + 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //     𝛾̿ⱼ ← 𝛾̿ⱼ + 𝜏ⱼᵢ⋅𝛾ᵢ₊₁.
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      omega_ = gamma_[l] = gamma_bar_[l], rho_ *= -omega_;
      for (size_t k = l - 1; k != 0; --k) {
        gamma_[k] = gamma_bar_[k];
        for (size_t i{k + 1}; i <= l; ++i) {
          gamma_[k] -= tau_[k, i] * gamma_[i];
        }
      }
      for (size_t k = 1; k < l; ++k) {
        gamma_bbar_[k] = gamma_[k + 1];
        for (size_t i = k + 1; i < l; ++i) {
          gamma_bbar_[k] += tau_[k, i] * gamma_[i + 1];
        }
      }

      // Update the solution and the residual again:
      // ----------------------
      // 𝒙 ← 𝒙 + 𝛾₁⋅𝒓₀,
      // 𝒓₀ ← 𝒓₀ - 𝛾̅ₗ⋅𝒓ₗ,
      // 𝒖₀ ← 𝒖₀ - 𝛾ₗ⋅𝒖ₗ,
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //   𝒙 ← 𝒙 + 𝛾̿ⱼ⋅𝒓ⱼ,
      //   𝒓₀ ← 𝒓₀ - 𝛾̅ⱼ⋅𝒓ⱼ,
      //   𝒖₀ ← 𝒖₀ - 𝛾ⱼ⋅𝒖ⱼ.
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      Blas::AddAssign(x, rs_[0], gamma_[1]);
      Blas::SubAssign(rs_[0], rs_[l], gamma_bar_[l]);
      Blas::SubAssign(us_[0], us_[l], gamma_[l]);
      for (size_t k = 1; k < l; ++k) {
        Blas::AddAssign(x, rs_[k], gamma_bbar_[k]);
        Blas::SubAssign(rs_[0], rs_[k], gamma_bar_[k]);
        Blas::SubAssign(us_[0], us_[k], gamma_[k]);
      }
    }

    return Blas::Norm2(rs_[0]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  real_t alpha_{}, rho_{}, omega_{};
  std::vector<real_t> gamma_, gamma_bar_, gamma_bbar_, sigma_;
  Mdvector<real_t, 2> tau_;
  Vector r_tilde_, z_;
  std::vector<Vector> rs_, us_;

  friend class InnerOuterIterativeSolver<Vector>;

}; // class BiCGStabL

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
