/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// Copyright (C) 2022 Oleg Butakov
///
/// Permission is hereby granted, free of charge, to any person
/// obtaining a copy of this software and associated documentation
/// files (the "Software"), to deal in the Software without
/// restriction, including without limitation the rights  to use,
/// copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the
/// Software is furnished to do so, subject to the following
/// conditions:
///
/// The above copyright notice and this permission notice shall be
/// included in all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/// OTHER DEALINGS IN THE SOFTWARE.
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///

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

/// ----------------------------------------------------------------- ///
/// @brief Base class for @c GMRES, @c FGMRES,
///   @c LGMRES and @c LFGMRES.
/// ----------------------------------------------------------------- ///
template<VectorLike Vector, bool Flexible, bool Loose = false>
class BaseGmresSolver_ : public InnerOuterIterativeSolver<Vector> {
private:

  std::vector<real_t> beta_, cs_, sn_;
  Mdvector<real_t, 2> H_;
  std::vector<Vector> qVecs_;
  std::vector<Vector> zVecs_;

  auto OuterInit(const Vector& xVec,
                 const Vector& bVec,
                 const Operator<Vector>& linOp,
                 const Preconditioner<Vector>* preOp) -> real_t override {
    const size_t m{this->NumInnerIterations};

    beta_.resize(m + 1);
    cs_.resize(m), sn_.resize(m);
    H_.assign(m + 1, m);

    qVecs_.resize(m + 1);
    zVecs_.resize(Flexible && preOp != nullptr ? m + 1 : 1);
    for (Vector& qVec : qVecs_) qVec.Assign(xVec, false);
    for (Vector& zVec : zVecs_) zVec.Assign(xVec, false);

    /// @todo Refactor without duplication a code from
    ///   InnerInit method.
    const bool leftPre{(preOp != nullptr) && (!Flexible) &&
                       (this->PreSide == PreconditionerSide::Left)};

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
    linOp.Residual(qVecs_[0], bVec, xVec);
    if (leftPre) {
      Blas::Swap(zVecs_[0], qVecs_[0]);
      preOp->MatVec(qVecs_[0], zVecs_[0]);
    }
    beta_[0] = Blas::Norm2(qVecs_[0]);
    Blas::ScaleAssign(qVecs_[0], 1.0 / beta_[0]);

    return beta_[0];
  }

  void InnerInit(const Vector& xVec,
                 const Vector& bVec,
                 const Operator<Vector>& linOp,
                 const Preconditioner<Vector>* preOp) override {
    // Force right preconditioning for the flexible GMRES.
    const bool leftPre{(preOp != nullptr) && (!Flexible) &&
                       (this->PreSide == PreconditionerSide::Left)};

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
    linOp.Residual(qVecs_[0], bVec, xVec);
    if (leftPre) {
      Blas::Swap(zVecs_[0], qVecs_[0]);
      preOp->MatVec(qVecs_[0], zVecs_[0]);
    }
    beta_[0] = Blas::Norm2(qVecs_[0]);
    Blas::ScaleAssign(qVecs_[0], 1.0 / beta_[0]);
  }

  auto InnerIterate(Vector& /*xVec*/,
                    const Vector& /*bVec*/,
                    const Operator<Vector>& linOp,
                    const Preconditioner<Vector>* preOp) -> real_t override {
    const size_t k{this->InnerIteration};

    // Force right preconditioning for the flexible GMRES.
    const bool leftPre{
        (preOp != nullptr) &&
        (!Flexible && (this->PreSide == PreconditionerSide::Left))};
    const bool rightPre{
        (preOp != nullptr) &&
        (Flexible || (this->PreSide == PreconditionerSide::Right))};

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
    if (leftPre) {
      preOp->MatVec(qVecs_[k + 1], zVecs_[0], linOp, qVecs_[k]);
    } else if (rightPre) {
      const size_t j{Flexible ? k : 0};
      linOp.MatVec(qVecs_[k + 1], zVecs_[j], *preOp, qVecs_[k]);
    } else {
      linOp.MatVec(qVecs_[k + 1], qVecs_[k]);
    }
    for (size_t i{0}; i <= k; ++i) {
      H_[i, k] = Blas::Dot(qVecs_[k + 1], qVecs_[i]);
      Blas::SubAssign(qVecs_[k + 1], qVecs_[i], H_[i, k]);
    }
    H_[k + 1, k] = Blas::Norm2(qVecs_[k + 1]);
    Blas::ScaleAssign(qVecs_[k + 1], 1.0 / H_[k + 1, k]);

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
    for (size_t i{0}; i < k; ++i) {
      const real_t chi = cs_[i] * H_[i, k] + sn_[i] * H_[i + 1, k];
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

  void InnerFinalize(Vector& xVec,
                     const Vector& /*bVec*/,
                     const Operator<Vector>& /*linOp*/,
                     const Preconditioner<Vector>* preOp) override {
    const size_t k{this->InnerIteration};

    const bool rightPre{
        (preOp != nullptr) &&
        (Flexible || (this->PreSide == PreconditionerSide::Right))};

    // Finalize the 𝛽-solution:
    // ----------------------
    // 𝛽₀:ₖ ← (𝐻₀:ₖ,₀:ₖ)⁻¹𝛽₀:ₖ.
    // ----------------------
    for (size_t i{k}; i != SIZE_MAX; --i) {
      for (size_t j{i + 1}; j <= k; ++j) {
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
    if (!rightPre) {
      for (size_t i{0}; i <= k; ++i) {
        Blas::AddAssign(xVec, qVecs_[i], beta_[i]);
      }
    } else if constexpr (Flexible) {
      for (size_t i{0}; i <= k; ++i) {
        Blas::AddAssign(xVec, zVecs_[i], beta_[i]);
      }
    } else {
      Blas::ScaleAssign(qVecs_[0], beta_[0]);
      for (size_t i{1}; i <= k; ++i) {
        Blas::AddAssign(qVecs_[0], qVecs_[i], beta_[i]);
      }
      preOp->MatVec(zVecs_[0], qVecs_[0]);
      Blas::AddAssign(xVec, zVecs_[0]);
    }
  }

protected:

  BaseGmresSolver_() = default;

}; // class BaseGmresSolver_

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c GMRES (Generalized Minimal Residual)
///   linear operator equation solver.
///
/// @c GMRES is typically more robust than the @c BiCG type solvers,
///   but it may be slower than the @c BiCG solvers for the
///   well-conditioned moderate sized problems.
///
/// @c GMRES is algebraically equivalent to @c MINRES method
///   in the self-adjoint operator unpreconditioned case,
///   however, the need for restarts may lead to the much slower
///   @c GMRES convergence rate.
///
/// @c GMRES may be applied to the singular problems, and the square
///   least squares problems, although, similarly to @c MINRES,
///   convergeance to minimum norm solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef and Martin H. Schultz.
///     “GMRES: A generalized minimal residual algorithm for solving
///      nonsymmetric linear systems.”
///     SIAM J. Sci. Stat. Comput., 7:856–869, 1986.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class GmresSolver final : public BaseGmresSolver_<Vector, false> {};

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c FGMRES (Flexible Generalized Minimal Residual)
///   linear operator equation solver.
///
/// @c FGMRES is typically more robust than the @c BiCG type solvers,
///   but it may be slower than the @c BiCG solvers for the
///   well-conditioned moderate sized problems.
///
/// @c FGMRES does the same amount of operations per iteration
///   as @c GMRES, but also allows usage of the variable (or flexible)
///   preconditioners with the price of doubleing of the memory
///   usage. For the static preconditioners, @c FGMRES requires
///   one preconditioner-vector product less than @c GMRES.
///   @c FGMRES supports only the right preconditioning.
///
/// @c FGMRES may be applied to the singular problems, and the square
///   least squares problems, although, similarly to @c MINRES,
///   convergeance to minimum norm solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef.
///     “A Flexible Inner-Outer Preconditioned GMRES Algorithm.”
///     SIAM J. Sci. Comput. 14 (1993): 461-469.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class FgmresSolver final : public BaseGmresSolver_<Vector, true> {};

} // namespace tit::ksp
