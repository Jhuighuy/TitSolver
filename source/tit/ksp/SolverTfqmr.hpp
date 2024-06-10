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

#include "tit/core/math.hpp"

#include "tit/ksp/Solver.hpp"
#include "tit/ksp/Vector.hpp"

namespace tit::ksp {

/// ----------------------------------------------------------------- ///
/// @brief Base class for @c TFQMR and @c TFQMR1.
/// ----------------------------------------------------------------- ///
template<VectorLike Vector, bool L1>
class BaseTfqmrSolver_ : public IterativeSolver<Vector> {
private:

  real_t rho_, tau_;
  Vector dVec_, rTildeVec_, uVec_, vVec_, yVec_, sVec_, zVec_;

  real_t Init(const Vector& xVec,
              const Vector& bVec,
              const Operator<Vector>& linOp,
              const Preconditioner<Vector>* preOp) override;

  real_t Iterate(Vector& xVec,
                 const Vector& bVec,
                 const Operator<Vector>& linOp,
                 const Preconditioner<Vector>* preOp) override;

protected:

  BaseTfqmrSolver_() = default;

}; // class BaseTfqmrSolver_

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c TFQMR (Transpose-Free Quasi-Minimal Residual)
///   linear operator equation solver.
///
/// @c TFQMR, like the other @c BiCG type methods, normally
///   requires two operator-vector products per iteration.
///   But, unlike the other @c BiCG type methods, @c TFQMR does not
///   implicitly contain the residual norm estimate, only the rough
///   upper bound is avariable, so at the latter iterations an extra
///   operator-vector product per iteration may be required for the
///   explicit residual estimation.
///
/// @c TFQMR typically converges much smoother, than
///   @c CGS and @c BiCGStab. @todo Breakdowns?
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
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class TfqmrSolver final : public BaseTfqmrSolver_<Vector, false> {};

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c TFQMR1 (Transpose-Free 1-norm
///   Quasi-Minimal Residual) linear operator equation solver.
///
/// @c TFQMR1, like the other @c BiCG type solvers, requires
///   two operator-vector products per iteration. Unlike @c TFQMR,
///   @c TFQMR1 implicitly contains the residual norm estimate, so no
///   extra operator-vector products are required.
///
/// @c TFQMR1 typically converges much smoother, than
///   @c CGS and @c BiCGStab and is slightly faster than
///   @c TFQMR. @todo Breakdowns?
///
/// References:
/// @verbatim
/// [1] H.M Bücker,
///     “A Transpose-Free 1-norm Quasi-Minimal Residual Algorithm
///      for Non-Hermitian Linear Systems.“, FZJ-ZAM-IB-9706.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class Tfqmr1Solver final : public BaseTfqmrSolver_<Vector, true> {};

template<VectorLike Vector, bool L1>
real_t BaseTfqmrSolver_<Vector, L1>::Init(const Vector& xVec,
                                          const Vector& bVec,
                                          const Operator<Vector>& linOp,
                                          const Preconditioner<Vector>* preOp) {
  const bool leftPre{(preOp != nullptr) &&
                     (this->PreSide == PreconditionerSide::Left)};

  dVec_.Assign(xVec, false);
  rTildeVec_.Assign(xVec, false);
  uVec_.Assign(xVec, false);
  vVec_.Assign(xVec, false);
  yVec_.Assign(xVec, false);
  sVec_.Assign(xVec, false);
  if (preOp != nullptr) {
    zVec_.Assign(xVec, false);
  }

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
    Blas::Set(dVec_, xVec);
  } else {
    Blas::Fill(dVec_, 0.0);
  }
  linOp.Residual(yVec_, bVec, xVec);
  if (leftPre) {
    Blas::Swap(zVec_, yVec_);
    preOp->MatVec(yVec_, zVec_);
  }
  Blas::Set(uVec_, yVec_);
  Blas::Set(rTildeVec_, uVec_);
  rho_ = Blas::Dot(rTildeVec_, uVec_), tau_ = sqrt(rho_);

  return tau_;

} // BaseTfqmrSolver_::Init

template<VectorLike Vector, bool L1>
real_t BaseTfqmrSolver_<Vector, L1>::Iterate(
    Vector& xVec,
    const Vector& bVec,
    const Operator<Vector>& linOp,
    const Preconditioner<Vector>* preOp) {
  const bool leftPre{(preOp != nullptr) &&
                     (this->PreSide == PreconditionerSide::Left)};
  const bool rightPre{(preOp != nullptr) &&
                      (this->PreSide == PreconditionerSide::Right)};

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
  const bool firstIteration{this->Iteration == 0};
  if (firstIteration) {
    if (leftPre) {
      preOp->MatVec(sVec_, zVec_, linOp, yVec_);
    } else if (rightPre) {
      linOp.MatVec(sVec_, zVec_, *preOp, yVec_);
    } else {
      linOp.MatVec(sVec_, yVec_);
    }
    Blas::Set(vVec_, sVec_);
  } else {
    const real_t rhoBar{rho_};
    rho_ = Blas::Dot(rTildeVec_, uVec_);
    const real_t beta{safe_divide(rho_, rhoBar)};
    Blas::Add(vVec_, sVec_, vVec_, beta);
    Blas::Add(yVec_, uVec_, yVec_, beta);
    if (leftPre) {
      preOp->MatVec(sVec_, zVec_, linOp, yVec_);
    } else if (rightPre) {
      linOp.MatVec(sVec_, zVec_, *preOp, yVec_);
    } else {
      linOp.MatVec(sVec_, yVec_);
    }
    Blas::Add(vVec_, sVec_, vVec_, beta);
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
  const real_t alpha{safe_divide(rho_, Blas::Dot(rTildeVec_, vVec_))};
  for (size_t m{0}; m <= 1; ++m) {
    Blas::SubAssign(uVec_, sVec_, alpha);
    Blas::AddAssign(dVec_, rightPre ? zVec_ : yVec_, alpha);
    const real_t omega{Blas::Norm2(uVec_)};
    if constexpr (L1) {
      if (omega < tau_) {
        tau_ = omega, Blas::Set(xVec, dVec_);
      }
    } else {
      const auto [cs, sn, rr] = sym_ortho(tau_, omega);
      tau_ = omega * cs;
      Blas::AddAssign(xVec, dVec_, std::pow(cs, 2));
      Blas::ScaleAssign(dVec_, std::pow(sn, 2));
    }
    if (m == 0) {
      Blas::SubAssign(yVec_, vVec_, alpha);
      if (leftPre) {
        preOp->MatVec(sVec_, zVec_, linOp, yVec_);
      } else if (rightPre) {
        linOp.MatVec(sVec_, zVec_, *preOp, yVec_);
      } else {
        linOp.MatVec(sVec_, yVec_);
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
  real_t tauTilde{tau_};
  if constexpr (!L1) {
    const size_t k{this->Iteration};
    tauTilde *= sqrt(2.0 * k + 3.0);
  }

  return tauTilde;

} // BaseTfqmrSolver_::Iterate

} // namespace tit::ksp
