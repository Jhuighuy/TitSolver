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

#include <limits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/Operator.hpp"
#include "tit/ksp/Preconditioner.hpp"
#include "tit/ksp/Solver.hpp"
#include "tit/ksp/SolverBiCgStab.hpp"
#include "tit/ksp/Vector.hpp"

namespace tit::ksp {

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The @c Newton method nonlinear operator equation solver.
///
/// The classical Newton iterations are based on the linearization
/// of 𝓐(𝒙) near 𝒙:
///
/// 𝓐(𝒙̂) ≈ 𝓐(𝒙) + [∂𝓐(𝒙)/∂𝒙](𝒙̂ - 𝒙) = 𝒃,
///
/// or, alternatively:
///
/// [∂𝓐(𝒙)/∂𝒙]𝒕 = 𝒓, 𝒕 = 𝒙̂ - 𝒙, 𝒓 = 𝒃 - 𝓐(𝒙)
///
/// where 𝒙 and 𝒙̂ are the current and updated solution vectors.
/// Therefore, a linear equation has to be solved on each iteration,
/// linear operator 𝓙(𝒙) ≈ ∂𝓐(𝒙)/∂𝒙 for computing Jacobian-vector
/// products is required.
///
/// References:
/// @verbatim
/// [1] ???
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class NewtonSolver : public IterativeSolver<Vector> {
private:

  auto Init(const Vector& /*xVec*/,
            const Vector& /*bVec*/,
            const Operator<Vector>& /*anyOp*/,
            const Preconditioner<Vector>* /*preOp*/) -> real_t override {
    TIT_ENSURE(false, "Newton solver was not implemented yet!");
    return 0.0;
  }

  auto Iterate(Vector& /*xVec*/,
               const Vector& /*bVec*/,
               const Operator<Vector>& /*anyOp*/,
               const Preconditioner<Vector>* /*preOp*/) -> real_t override {
    TIT_ENSURE(false, "Newton solver was not implemented yet!");
    return 0.0;
  }

}; // class NewtonSolver

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief The first-order @c JFNK (Jacobian free-Newton-Krylov)
///   nonlinear operator equation solver.
///
/// For the @c Newton iterations, computing of the Jacobian-vector
/// products 𝒛 = 𝓙(𝒙)𝒚, where 𝓙(𝒙) ≈ ∂𝓐(𝒙)/∂𝒙 is required.
/// Consider the expansion:
///
/// 𝓐(𝒙 + 𝛿⋅𝒚) = 𝓐(𝒙) + 𝛿⋅[∂𝓐(𝒙)/∂𝒙]𝒚 + 𝓞(𝛿²),
///
/// where 𝛿 is some small number. Therefore,
///
/// 𝓙(𝒙)𝒚 = [𝓐(𝒙 + 𝛿⋅𝒚) - 𝓐(𝒙)]/𝛿 = [∂𝓐(𝒙)/∂𝒙]𝒚 + 𝓞(𝛿).
///
/// Expression above may be used as the formula for computing
/// the (approximate) Jacobian-vector products. Parameter 𝛿 is commonly
/// defined as [1]:
///
/// 𝛿 = 𝜇⋅‖𝒚‖⁺, 𝜇 = (𝜀ₘ)¹ᐟ²⋅(1 + ‖𝒙‖)¹ᐟ²,
///
/// where 𝜀ₘ is the machine roundoff, ‖𝒚‖⁺ is the pseudo-inverse to ‖𝒚‖.
///
/// References:
/// @verbatim
/// [1] Liu, Wei, Lilun Zhang, Ying Zhong, Yongxian Wang,
///     Yonggang Che, Chuanfu Xu and Xinghua Cheng.
///     “CFD High-order Accurate Scheme JFNK Solver.”
///     Procedia Engineering 61 (2013): 9-15.
/// @endverbatim
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike Vector>
class JfnkSolver final : public IterativeSolver<Vector> {
private:

  Vector sVec_, tVec_, rVec_, wVec_;

  auto Init(const Vector& xVec,
            const Vector& bVec,
            const Operator<Vector>& anyOp,
            const Preconditioner<Vector>* /*preOp*/) -> real_t override {
    sVec_.Assign(xVec, false);
    tVec_.Assign(xVec, false);
    rVec_.Assign(xVec, false);
    wVec_.Assign(xVec, false);

    // Initialize:
    // ----------------------
    // 𝒘 ← 𝓐(𝒙),
    // 𝒓 ← 𝒃 - 𝒘.
    // ----------------------
    anyOp.MatVec(wVec_, xVec);
    Blas::Sub(rVec_, bVec, wVec_);

    return Blas::Norm2(rVec_);
  }

  auto Iterate(Vector& xVec,
               const Vector& bVec,
               const Operator<Vector>& anyOp,
               const Preconditioner<Vector>* /*preOp*/) -> real_t override {
    // Solve the Jacobian equation:
    // ----------------------
    // 𝜇 ← (𝜀ₘ)¹ᐟ²⋅(1 + ‖𝒙‖)]¹ᐟ²,
    // 𝒕 ← 𝒓,
    // 𝒕 ← 𝓙(𝒙)⁻¹𝒓.
    // ----------------------
    static const real_t sqrtOfEpsilon{
        sqrt(std::numeric_limits<real_t>::epsilon())};
    const real_t mu{sqrtOfEpsilon * sqrt(1.0 + Blas::Norm2(xVec))};
    Blas::Set(tVec_, rVec_);
    {
      auto solver = std::make_unique<BiCgStabSolver<Vector>>();
      solver->AbsoluteTolerance = 1.0e-8;
      solver->RelativeTolerance = 1.0e-8;
      auto op = MakeOperator<Vector>([&](Vector& zVec, const Vector& yVec) {
        // Compute the Jacobian-vector product:
        // ----------------------
        // 𝛿 ← 𝜇⋅‖𝒚‖⁺,
        // 𝒔 ← 𝒙 + 𝛿⋅𝒚,
        // 𝒛 ← 𝓐(𝒔),
        // 𝒛 ← 𝛿⁺⋅𝒛 - 𝛿⁺⋅𝒘.
        // ----------------------
        const real_t delta{safe_divide(mu, Blas::Norm2(yVec))};
        Blas::Add(sVec_, xVec, yVec, delta);
        anyOp.MatVec(zVec, sVec_);
        const real_t deltaInverse{safe_divide(1.0, delta)};
        Blas::Sub(zVec, zVec, deltaInverse, wVec_, deltaInverse);
      });
      solver->Solve(tVec_, rVec_, *op);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝒙 ← 𝒙 + 𝒕,
    // 𝒘 ← 𝓐(𝒙),
    // 𝒓 ← 𝒃 - 𝒘.
    // ----------------------
    Blas::AddAssign(xVec, tVec_);
    anyOp.MatVec(wVec_, xVec);
    Blas::Sub(rVec_, bVec, wVec_);

    return Blas::Norm2(rVec_);
  }

}; // class JfnkSolver

} // namespace tit::ksp
