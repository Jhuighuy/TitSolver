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

#include <cmath>

#include "tit/ksp/LegacyTensor.hpp"
#include "tit/ksp/Solver.hpp"
#include "tit/ksp/Subspace.hpp"
#include "tit/ksp/Vector.hpp"
#include "tit/ksp/stormBase.hpp"

namespace tit::ksp {

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c BiCGStab (Biconjugate Gradients Stabilized)
///   linear operator equation solver.
///
/// @c BiCGStab, like the other @c BiCG type solvers, requires
///   two operator multiplications per iteration.
///
/// @c BiCGStab typically converges much smoother, than
///   @c CGS. @todo Breakdowns?
///
/// References:
/// @verbatim
/// [1] Henk A. van der Vorst.
///     “Bi-CGSTAB: A Fast and Smoothly Converging Variant of Bi-CG
///      for the Solution of Nonsymmetric Linear Systems.”
///     SIAM J. Sci. Comput. 13 (1992): 631-644.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class BiCgStabSolver final : public IterativeSolver<Vector> {
private:

  real_t alpha_, rho_, omega_;
  Vector pVec_, rVec_, rTildeVec_, tVec_, vVec_, zVec_;

  real_t Init(const Vector& xVec,
              const Vector& bVec,
              const Operator<Vector>& linOp,
              const Preconditioner<Vector>* preOp) override;

  real_t Iterate(Vector& xVec,
                 const Vector& bVec,
                 const Operator<Vector>& linOp,
                 const Preconditioner<Vector>* preOp) override;

}; // class BiCgStabSolver

template<VectorLike Vector>
real_t BiCgStabSolver<Vector>::Init(const Vector& xVec,
                                    const Vector& bVec,
                                    const Operator<Vector>& linOp,
                                    const Preconditioner<Vector>* preOp) {
  const bool leftPre{(preOp != nullptr) &&
                     (this->PreSide == PreconditionerSide::Left)};

  pVec_.Assign(xVec, false);
  rVec_.Assign(xVec, false);
  rTildeVec_.Assign(xVec, false);
  tVec_.Assign(xVec, false);
  vVec_.Assign(xVec, false);
  if (preOp != nullptr) {
    zVec_.Assign(xVec, false);
  }

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
  linOp.Residual(rVec_, bVec, xVec);
  if (leftPre) {
    Blas::Swap(zVec_, rVec_);
    preOp->MatVec(rVec_, zVec_);
  }
  Blas::Set(rTildeVec_, rVec_);
  rho_ = Blas::Dot(rTildeVec_, rVec_);

  return std::sqrt(rho_);

} // BiCgStabSolver::Init

template<VectorLike Vector>
real_t BiCgStabSolver<Vector>::Iterate(Vector& xVec,
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
  //   𝒑 ← 𝒓.
  // 𝗲𝗹𝘀𝗲:
  //   𝜌̅ ← 𝜌,
  //   𝜌 ← <𝒓̃⋅𝒓>,
  //   𝛽 ← (𝜌/𝜌̅)⋅(𝛼/𝜔),
  //   𝒑 ← 𝒑 - 𝜔⋅𝒗,
  //   𝒑 ← 𝒓 + 𝛽⋅𝒑.
  // 𝗲𝗻𝗱 𝗶𝗳
  // ----------------------
  const bool firstIteration{this->Iteration == 0};
  if (firstIteration) {
    Blas::Set(pVec_, rVec_);
  } else {
    const real_t rhoBar{rho_};
    rho_ = Blas::Dot(rTildeVec_, rVec_);
    const real_t beta{Utils::SafeDivide(alpha_ * rho_, omega_ * rhoBar)};
    Blas::SubAssign(pVec_, vVec_, omega_);
    Blas::Add(pVec_, rVec_, pVec_, beta);
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
  if (leftPre) {
    preOp->MatVec(vVec_, zVec_, linOp, pVec_);
  } else if (rightPre) {
    linOp.MatVec(vVec_, zVec_, *preOp, pVec_);
  } else {
    linOp.MatVec(vVec_, pVec_);
  }
  alpha_ = Utils::SafeDivide(rho_, Blas::Dot(rTildeVec_, vVec_));
  Blas::AddAssign(xVec, rightPre ? zVec_ : pVec_, alpha_);
  Blas::SubAssign(rVec_, vVec_, alpha_);

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
  if (leftPre) {
    preOp->MatVec(tVec_, zVec_, linOp, rVec_);
  } else if (rightPre) {
    linOp.MatVec(tVec_, zVec_, *preOp, rVec_);
  } else {
    linOp.MatVec(tVec_, rVec_);
  }
  omega_ = Utils::SafeDivide(Blas::Dot(tVec_, rVec_), Blas::Dot(tVec_, tVec_));
  Blas::AddAssign(xVec, rightPre ? zVec_ : rVec_, omega_);
  Blas::SubAssign(rVec_, tVec_, omega_);

  return Blas::Norm2(rVec_);

} // BiCgStabSolver::Iterate

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c BiCGStab(l) (Biconjugate Gradients Stabilized)
///   linear operator equation solver.
///
/// @c BiCGStab(l), like the other @c BiCG type solvers, requires
///   two operator multiplications per iteration.
///
/// References:
/// @verbatim
/// [1] Gerard L. G. Sleijpen and Diederik R. Fokkema.
///     “BiCGStab(l) for Linear Equations involving
///      Unsymmetric Matrices with Complex Spectrum.”
///     Electronic Transactions on Numerical Analysis 1 (1993): 11-32.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class BiCgStabLSolver final : public InnerOuterIterativeSolver<Vector> {
private:

  real_t alpha_, rho_, omega_;
  stormVector<real_t> gamma_, gammaBar_, gammaBarBar_, sigma_;
  stormMatrix<real_t> tau_;
  Vector rTildeVec_, zVec_;
  Subspace<Vector> rVecs_, uVecs_;

  real_t OuterInit(const Vector& xVec,
                   const Vector& bVec,
                   const Operator<Vector>& linOp,
                   const Preconditioner<Vector>* preOp) override;

  real_t InnerIterate(Vector& xVec,
                      const Vector& bVec,
                      const Operator<Vector>& linOp,
                      const Preconditioner<Vector>* preOp) override;

public:

  BiCgStabLSolver() {
    this->NumInnerIterations = 2;
  }

}; // class BiCgStabLSolver

template<VectorLike Vector>
real_t BiCgStabLSolver<Vector>::OuterInit(const Vector& xVec,
                                          const Vector& bVec,
                                          const Operator<Vector>& linOp,
                                          const Preconditioner<Vector>* preOp) {
  const size_t l{this->NumInnerIterations};

  gamma_.Assign(l + 1);
  gammaBar_.Assign(l + 1);
  gammaBarBar_.Assign(l + 1);
  sigma_.Assign(l + 1);
  tau_.Assign(l + 1, l + 1);

  rTildeVec_.Assign(xVec, false);
  if (preOp != nullptr) {
    zVec_.Assign(xVec, false);
  }

  rVecs_.Assign(l + 1, xVec, false);
  uVecs_.Assign(l + 1, xVec, false);

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
  Blas::Fill(uVecs_(0), 0.0);
  linOp.Residual(rVecs_(0), bVec, xVec);
  if (preOp != nullptr) {
    Blas::Swap(zVec_, rVecs_(0));
    preOp->MatVec(rVecs_(0), zVec_);
  }
  Blas::Set(rTildeVec_, rVecs_(0));
  rho_ = Blas::Dot(rTildeVec_, rVecs_(0));

  return std::sqrt(rho_);

} // BiCgStabLSolver::OuterInit

template<VectorLike Vector>
real_t BiCgStabLSolver<Vector>::InnerIterate(
    Vector& xVec,
    const Vector& bVec,
    const Operator<Vector>& linOp,
    const Preconditioner<Vector>* preOp) {
  const size_t l{this->NumInnerIterations};
  const size_t j{this->InnerIteration};

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
  const bool firstIteration{this->Iteration == 0};
  if (firstIteration) {
    Blas::Set(uVecs_(0), rVecs_(0));
  } else {
    const real_t rhoBar{rho_};
    rho_ = Blas::Dot(rTildeVec_, rVecs_(j));
    const real_t beta{Utils::SafeDivide(alpha_ * rho_, rhoBar)};
    for (size_t i{0}; i <= j; ++i) {
      Blas::Sub(uVecs_(i), rVecs_(i), uVecs_(i), beta);
    }
  }
  if (preOp != nullptr) {
    preOp->MatVec(uVecs_(j + 1), zVec_, linOp, uVecs_(j));
  } else {
    linOp.MatVec(uVecs_(j + 1), uVecs_(j));
  }
  alpha_ = Utils::SafeDivide(rho_, Blas::Dot(rTildeVec_, uVecs_(j + 1)));
  for (size_t i{0}; i <= j; ++i) {
    Blas::SubAssign(rVecs_(i), uVecs_(i + 1), alpha_);
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
  Blas::AddAssign(xVec, uVecs_(0), alpha_);
  if (preOp != nullptr) {
    preOp->MatVec(rVecs_(j + 1), zVec_, linOp, rVecs_(j));
  } else {
    linOp.MatVec(rVecs_(j + 1), rVecs_(j));
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
    for (size_t j{1}; j <= l; ++j) {
      for (size_t i{1}; i < j; ++i) {
        tau_(i, j) =
            Utils::SafeDivide(Blas::Dot(rVecs_(i), rVecs_(j)), sigma_(i));
        Blas::SubAssign(rVecs_(j), rVecs_(i), tau_(i, j));
      }
      sigma_(j) = Blas::Dot(rVecs_(j), rVecs_(j));
      gammaBar_(j) =
          Utils::SafeDivide(Blas::Dot(rVecs_(0), rVecs_(j)), sigma_(j));
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
    omega_ = gamma_(l) = gammaBar_(l), rho_ *= -omega_;
    for (size_t j{l - 1}; j != 0; --j) {
      gamma_(j) = gammaBar_(j);
      for (size_t i{j + 1}; i <= l; ++i) {
        gamma_(j) -= tau_(j, i) * gamma_(i);
      }
    }
    for (size_t j{1}; j < l; ++j) {
      gammaBarBar_(j) = gamma_(j + 1);
      for (size_t i{j + 1}; i < l; ++i) {
        gammaBarBar_(j) += tau_(j, i) * gamma_(i + 1);
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
    Blas::AddAssign(xVec, rVecs_(0), gamma_(1));
    Blas::SubAssign(rVecs_(0), rVecs_(l), gammaBar_(l));
    Blas::SubAssign(uVecs_(0), uVecs_(l), gamma_(l));
    for (size_t j{1}; j < l; ++j) {
      Blas::AddAssign(xVec, rVecs_(j), gammaBarBar_(j));
      Blas::SubAssign(rVecs_(0), rVecs_(j), gammaBar_(j));
      Blas::SubAssign(uVecs_(0), uVecs_(j), gamma_(j));
    }
  }

  return Blas::Norm2(rVecs_(0));

} // BiCgStabLSolver::InnerIterate

} // namespace tit::ksp
