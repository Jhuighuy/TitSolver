/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>
#include <tuple>
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

namespace impl {

// Base class for GMRES, FGMRES, LGMRES and LFGMRES.
template<blas::vector Vector, bool Flexible>
class BaseGMRES : public InnerOuterIterativeSolver<Vector> {
protected:

  constexpr BaseGMRES() = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init(const Vector& x,
                            const Vector& b,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P)
      -> real_t override {
    const auto m = this->NumInnerIterations;

    beta_.resize(m + 1);
    cs_.resize(m), sn_.resize(m);
    H_.assign(m + 1, m);

    qs_.resize(m + 1);
    zs_.resize(Flexible && P != nullptr ? m + 1 : 1);
    for (Vector& q : qs_) q.Assign(x, false);
    for (Vector& z : zs_) z.Assign(x, false);

    /// @todo Refactor without duplication a code from inner_init method.
    const auto left_pre = (!Flexible) && (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    // Initialize:
    // ----------------------
    // 𝒒₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛₀ ← 𝒒₀,
    //   𝒒₀ ← 𝓟𝒛₀,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛽₀ ← ‖𝒒₀‖,
    // 𝒒₀ ← 𝒒₀/𝛽₀.
    // ----------------------
    A.Residual(qs_[0], b, x);
    if (left_pre) {
      Blas::Swap(zs_[0], qs_[0]);
      P->MatVec(qs_[0], zs_[0]);
    }
    beta_[0] = Blas::Norm2(qs_[0]);
    Blas::ScaleAssign(qs_[0], 1.0 / beta_[0]);

    return beta_[0];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr void inner_init(const Vector& x,
                            const Vector& b,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P) override {
    // Force right preconditioning for the flexible GMRES.
    const bool left_pre = (!Flexible) && (P != nullptr) && //
                          (this->PreSide == PreconditionerSide::Left);

    // Initialize:
    // ----------------------
    // 𝒒₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛₀ ← 𝒒₀,
    //   𝒒₀ ← 𝓟𝒛₀,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛽₀ ← ‖𝒒₀‖,
    // 𝒒₀ ← 𝒒₀/𝛽₀.
    // ----------------------
    A.Residual(qs_[0], b, x);
    if (left_pre) {
      Blas::Swap(zs_[0], qs_[0]);
      P->MatVec(qs_[0], zs_[0]);
    }
    beta_[0] = Blas::Norm2(qs_[0]);
    Blas::ScaleAssign(qs_[0], 1.0 / beta_[0]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto inner_iter(Vector& /*x*/,
                            const Vector& /*b*/,
                            const Operator<Vector>& A,
                            const Preconditioner<Vector>* P)
      -> real_t override {
    const auto k = this->InnerIteration;

    // Force right preconditioning for the flexible GMRES.
    const auto left_pre =
        (P != nullptr) &&
        (!Flexible && (this->PreSide == PreconditionerSide::Left));
    const auto right_pre =
        (P != nullptr) &&
        (Flexible || (this->PreSide == PreconditionerSide::Right));

    // Compute the new 𝒒ₖ₊₁ vector:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒒ₖ₊₁ ← 𝓟(𝒛₀ ← 𝓐𝒒ₖ),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝑗 ← 𝘍𝘭𝘦𝘹𝘪𝘣𝘭𝘦 ? 𝑘 : 𝟢,
    //   𝒒ₖ₊₁ ← 𝓐(𝒛ⱼ ← 𝓟𝒒ₖ),
    // 𝗲𝗹𝘀𝗲:
    //   𝒒ₖ₊₁ ← 𝓐𝒒ₖ,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //   𝐻ᵢₖ ← <𝒒ₖ₊₁⋅𝒒ᵢ>,
    //   𝒒ₖ₊₁ ← 𝒒ₖ₊₁ - 𝐻ᵢₖ⋅𝒒ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝐻ₖ₊₁,ₖ ← ‖𝒒ₖ₊₁‖,
    // 𝒒ₖ₊₁ ← 𝒒ₖ₊₁/𝐻ₖ₊₁,ₖ.
    // ----------------------
    if (left_pre) {
      P->MatVec(qs_[k + 1], zs_[0], A, qs_[k]);
    } else if (right_pre) {
      const auto j = Flexible ? k : 0;
      A.MatVec(qs_[k + 1], zs_[j], *P, qs_[k]);
    } else {
      A.MatVec(qs_[k + 1], qs_[k]);
    }
    for (size_t i = 0; i <= k; ++i) {
      H_[i, k] = Blas::Dot(qs_[k + 1], qs_[i]);
      Blas::SubAssign(qs_[k + 1], qs_[i], H_[i, k]);
    }
    H_[k + 1, k] = Blas::Norm2(qs_[k + 1]);
    Blas::ScaleAssign(qs_[k + 1], 1.0 / H_[k + 1, k]);

    // Eliminate the last element in 𝐻
    // and and update the rotation matrix:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 - 𝟣 𝗱𝗼:
    //   𝜒 ← 𝑐𝑠ᵢ⋅𝐻ᵢₖ + 𝑠𝑛ᵢ⋅𝐻ᵢ₊₁,ₖ,
    //   𝐻ᵢ₊₁,ₖ ← -𝑠𝑛ᵢ⋅𝐻ᵢₖ + 𝑐𝑠ᵢ⋅𝐻ᵢ₊₁,ₖ,
    //   𝐻ᵢₖ ← 𝜒,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝑐𝑠ₖ, 𝑠𝑛ₖ ← 𝘚𝘺𝘮𝘖𝘳𝘵𝘩𝘰(𝐻ₖₖ, 𝐻ₖ₊₁,ₖ),
    // 𝐻ₖₖ ← 𝑐𝑠ₖ⋅𝐻ₖₖ + 𝑠𝑛ₖ⋅𝐻ₖ₊₁,ₖ,
    // 𝐻ₖ₊₁,ₖ ← 𝟢.
    // ----------------------
    for (size_t i = 0; i < k; ++i) {
      const auto chi = cs_[i] * H_[i, k] + sn_[i] * H_[i + 1, k];
      H_[i + 1, k] = -sn_[i] * H_[i, k] + cs_[i] * H_[i + 1, k];
      H_[i, k] = chi;
    }
    std::tie(cs_[k], sn_[k], std::ignore) = sym_ortho(H_[k, k], H_[k + 1, k]);
    H_[k, k] = cs_[k] * H_[k, k] + sn_[k] * H_[k + 1, k];
    H_[k + 1, k] = 0.0;

    // Update the 𝛽-solution and the residual norm:
    // ----------------------
    // 𝛽ₖ₊₁ ← -𝑠𝑛ₖ⋅𝛽ₖ, 𝛽ₖ ← 𝑐𝑠ₖ⋅𝛽ₖ.
    // ----------------------
    beta_[k + 1] = -sn_[k] * beta_[k], beta_[k] *= cs_[k];

    return abs(beta_[k + 1]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr void inner_finalize(Vector& x,
                                const Vector& /*b*/,
                                const Operator<Vector>& /*A*/,
                                const Preconditioner<Vector>* P) override {
    const auto k = this->InnerIteration;

    const auto right_pre =
        (P != nullptr) &&
        (Flexible || (this->PreSide == PreconditionerSide::Right));

    // Finalize the 𝛽-solution:
    // ----------------------
    // 𝛽₀:ₖ ← (𝐻₀:ₖ,₀:ₖ)⁻¹𝛽₀:ₖ.
    // ----------------------
    for (size_t i = k; i != SIZE_MAX; --i) {
      for (size_t j = i + 1; j <= k; ++j) {
        beta_[i] -= H_[i, j] * beta_[j];
      }
      beta_[i] /= H_[i, i];
    }

    // Compute the 𝒙-solution:
    // ----------------------
    // 𝗶𝗳 𝗻𝗼𝘁 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //     𝒙 ← 𝒙 + 𝛽ᵢ⋅𝒒ᵢ.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘍𝘭𝘦𝘹𝘪𝘣𝘭𝘦:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //     𝒙 ← 𝒙 + 𝛽ᵢ⋅𝒛ᵢ.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲:
    //   𝒒₀ ← 𝛽₀⋅𝒒₀,
    //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑘 𝗱𝗼:
    //     𝒒₀ ← 𝒒₀ + 𝛽ᵢ⋅𝒒ᵢ,
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    //   𝒛₀ ← 𝓟𝒒₀,
    //   𝒙 ← 𝒙 + 𝒛₀.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (!right_pre) {
      for (size_t i = 0; i <= k; ++i) {
        Blas::AddAssign(x, qs_[i], beta_[i]);
      }
    } else if constexpr (Flexible) {
      for (size_t i = 0; i <= k; ++i) {
        Blas::AddAssign(x, zs_[i], beta_[i]);
      }
    } else {
      Blas::ScaleAssign(qs_[0], beta_[0]);
      for (size_t i = 1; i <= k; ++i) {
        Blas::AddAssign(qs_[0], qs_[i], beta_[i]);
      }
      P->MatVec(zs_[0], qs_[0]);
      Blas::AddAssign(x, zs_[0]);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::vector<real_t> beta_, cs_, sn_;
  Mdvector<real_t, 2> H_;
  std::vector<Vector> qs_;
  std::vector<Vector> zs_;

}; // class BaseGMRES

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The GMRES (Generalized Minimal Residual) linear operator equation solver.
///
/// GMRES is typically more robust than the BiCG type solvers, but it may be
/// slower than the BiCG solvers for the well-conditioned moderate sized
/// problems.
///
/// GMRES is algebraically equivalent to MINRES method in the self-adjoint
/// operator unpreconditioned case, however, the need for restarts may lead to
/// the much slower GMRES convergence rate.
///
/// GMRES may be applied to the singular problems, and the square least squares
/// problems, although, similarly to MINRES, convergeance to minimum norm
/// solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef and Martin H. Schultz.
///     “GMRES: A generalized minimal residual algorithm for solving
///      nonsymmetric linear systems.”
///     SIAM J. Sci. Stat. Comput., 7:856–869, 1986.
/// @endverbatim
template<blas::vector Vector>
class GMRES final : public impl::BaseGMRES<Vector, /*Flexible=*/false> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The FGMRES (Flexible Generalized Minimal Residual) linear operator equation
/// solver.
///
/// FGMRES is typically more robust than the BiCG type solvers, but it may be
/// slower than the BiCG solvers for the well-conditioned moderate sized
/// problems.
///
/// FGMRES does the same amount of operations per iteration as GMRES, but also
/// allows usage of the variable (or flexible) preconditioners with the price of
/// doubleing of the memory usage. For the static preconditioners, FGMRES
/// requires one preconditioner-vector product less than @c GMRES. FGMRES
/// supports only the right preconditioning.
///
/// FGMRES may be applied to the singular problems, and the square least squares
/// problems, although, similarly to MINRES, convergeance to minimum norm
/// solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef.
///     “A Flexible Inner-Outer Preconditioned GMRES Algorithm.”
///     SIAM J. Sci. Comput. 14 (1993): 461-469.
/// @endverbatim
template<blas::vector Vector>
class FGMRES final : public impl::BaseGMRES<Vector, /*Flexible=*/true> {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
