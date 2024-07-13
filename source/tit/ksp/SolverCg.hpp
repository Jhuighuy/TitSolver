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

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c CG (Conjugate Gradients) linear self-adjoint
///   definite operator equation solver.
///
/// @c CG may be applied to the consistent singular problems,
/// it converges towards..
///
/// References:
/// @verbatim
/// [1] Hestenes, Magnus R. and Eduard Stiefel.
///     “Methods of conjugate gradients for solving linear systems.”
///     Journal of research of the National
///     Bureau of Standards 49 (1952): 409-435.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class CgSolver final : public IterativeSolver<Vector> {
private:

  real_t gamma_;
  Vector pVec_, rVec_, zVec_;

  real_t Init(const Vector& xVec,
              const Vector& bVec,
              const Operator<Vector>& linOp,
              const Preconditioner<Vector>* preOp) override;

  real_t Iterate(Vector& xVec,
                 const Vector& bVec,
                 const Operator<Vector>& linOp,
                 const Preconditioner<Vector>* preOp) override;

}; // class CgSolver

template<VectorLike Vector>
real_t CgSolver<Vector>::Init(const Vector& xVec,
                              const Vector& bVec,
                              const Operator<Vector>& linOp,
                              const Preconditioner<Vector>* preOp) {
  pVec_.Assign(xVec, false);
  rVec_.Assign(xVec, false);
  zVec_.Assign(xVec, false);

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
  linOp.Residual(rVec_, bVec, xVec);
  if (preOp != nullptr) {
    preOp->MatVec(zVec_, rVec_);
    Blas::Set(pVec_, zVec_);
    gamma_ = Blas::Dot(rVec_, zVec_);
  } else {
    Blas::Set(pVec_, rVec_);
    gamma_ = Blas::Dot(rVec_, rVec_);
  }

  return (preOp != nullptr) ? Blas::Norm2(rVec_) : sqrt(gamma_);

} // CgSolver::Init

template<VectorLike Vector>
real_t CgSolver<Vector>::Iterate(Vector& xVec,
                                 const Vector& bVec,
                                 const Operator<Vector>& linOp,
                                 const Preconditioner<Vector>* preOp) {
  // Iterate:
  // ----------------------
  // 𝒛 ← 𝓐𝒑,
  // 𝛾̅ ← 𝛾,
  // 𝛼 ← 𝛾/<𝒑⋅𝒛>,
  // 𝒙 ← 𝒙 + 𝛼⋅𝒑,
  // 𝒓 ← 𝒓 - 𝛼⋅𝒛.
  // ----------------------
  linOp.MatVec(zVec_, pVec_);
  const real_t gammaBar{gamma_};
  const real_t alpha = safe_divide(gamma_, Blas::Dot(pVec_, zVec_));
  Blas::AddAssign(xVec, pVec_, alpha);
  Blas::SubAssign(rVec_, zVec_, alpha);

  // ----------------------
  // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
  //   𝒛 ← 𝓟𝒓,
  //   𝛾 ← <𝒓⋅𝒛>,
  // 𝗲𝗹𝘀𝗲:
  //   𝛾 ← <𝒓⋅𝒓>.
  // 𝗲𝗻𝗱 𝗶𝗳
  // ----------------------
  if (preOp != nullptr) {
    preOp->MatVec(zVec_, rVec_);
    gamma_ = Blas::Dot(rVec_, zVec_);
  } else {
    gamma_ = Blas::Dot(rVec_, rVec_);
  }

  // ----------------------
  // 𝛽 ← 𝛾/𝛾̅,
  // 𝒑 ← (𝓟 ≠ 𝗻𝗼𝗻𝗲 ? 𝒛 : 𝒓) + 𝛽⋅𝒑.
  // ----------------------
  const real_t beta = safe_divide(gamma_, gammaBar);
  Blas::Add(pVec_, preOp != nullptr ? zVec_ : rVec_, pVec_, beta);

  return (preOp != nullptr) ? Blas::Norm2(rVec_) : sqrt(gamma_);

} // CgSolver::Iterate

} // namespace tit::ksp
