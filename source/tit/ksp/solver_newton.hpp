/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <limits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"
#include "tit/ksp/solver.hpp"
#include "tit/ksp/solver_bicgstab.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The Newton method nonlinear operator equation solver.
///
/// The classical Newton iterations are based on the linearization of 𝓐(𝒙) near
/// 𝒙:
///
/// 𝓐(𝒙̂) ≈ 𝓐(𝒙) + [∂𝓐(𝒙)/∂𝒙](𝒙̂ - 𝒙) = 𝒃,
///
/// or, alternatively:
///
/// [∂𝓐(𝒙)/∂𝒙]𝒕 = 𝒓, 𝒕 = 𝒙̂ - 𝒙, 𝒓 = 𝒃 - 𝓐(𝒙)
///
/// where 𝒙 and 𝒙̂ are the current and updated solution vectors. Therefore, a
/// linear equation has to be solved on each iteration, linear operator 𝓙(𝒙) ≈
/// ∂𝓐(𝒙)/∂𝒙 for computing Jacobian-vector products is required.
template<blas::vector Vector>
class NewtonSolver : public IterativeSolver<Vector> {
private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& /*x*/,
                      const Vector& /*b*/,
                      const Operator<Vector>& /*A*/,
                      const Preconditioner<Vector>* /*P*/) -> real_t override {
    TIT_ENSURE(false, "Newton solver was not implemented yet!");
    return 0.0;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto iterate(Vector& /*x*/,
                         const Vector& /*b*/,
                         const Operator<Vector>& /*A*/,
                         const Preconditioner<Vector>* /*P*/)
      -> real_t override {
    TIT_ENSURE(false, "Newton solver was not implemented yet!");
    return 0.0;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class NewtonSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The first-order JFNK (Jacobian free-Newton-Krylov) nonlinear operator
/// equation solver.
///
/// For the Newton iterations, computing of the Jacobian-vector products
/// 𝒛 = 𝓙(𝒙)𝒚, where 𝓙(𝒙) ≈ ∂𝓐(𝒙)/∂𝒙 is required. Consider the expansion:
///
/// 𝓐(𝒙 + 𝛿⋅𝒚) = 𝓐(𝒙) + 𝛿⋅[∂𝓐(𝒙)/∂𝒙]𝒚 + 𝓞(𝛿²),
///
/// where 𝛿 is some small number. Therefore,
///
/// 𝓙(𝒙)𝒚 = [𝓐(𝒙 + 𝛿⋅𝒚) - 𝓐(𝒙)]/𝛿 = [∂𝓐(𝒙)/∂𝒙]𝒚 + 𝓞(𝛿).
///
/// Expression above may be used as the formula for computing  the (approximate)
/// Jacobian-vector products. Parameter 𝛿 is commonly defined as [1]:
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
template<blas::vector Vector>
class JFNK final : public IterativeSolver<Vector> {
private:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init(const Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* /*P*/) -> real_t override {
    s_.Assign(x, false);
    t_.Assign(x, false);
    r_.Assign(x, false);
    w_.Assign(x, false);

    // Initialize:
    // ----------------------
    // 𝒘 ← 𝓐(𝒙),
    // 𝒓 ← 𝒃 - 𝒘.
    // ----------------------
    A.MatVec(w_, x);
    Blas::Sub(r_, b, w_);

    return Blas::Norm2(r_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto iter(Vector& x,
                      const Vector& b,
                      const Operator<Vector>& A,
                      const Preconditioner<Vector>* /*P*/) -> real_t override {
    // Solve the Jacobian equation:
    // ----------------------
    // 𝜇 ← (𝜀ₘ)¹ᐟ²⋅(1 + ‖𝒙‖)]¹ᐟ²,
    // 𝒕 ← 𝒓,
    // 𝒕 ← 𝓙(𝒙)⁻¹𝒓.
    // ----------------------
    const auto sqrt_of_eps = sqrt(std::numeric_limits<real_t>::epsilon());
    const auto mu = sqrt_of_eps * sqrt(1.0 + Blas::Norm2(x));
    Blas::Set(t_, r_);
    {
      auto solver = std::make_unique<BiCGStab<Vector>>();
      solver->AbsoluteTolerance = 1.0e-8;
      solver->RelativeTolerance = 1.0e-8;
      auto op = MakeOperator<Vector>([&](Vector& z, const Vector& y) {
        // Compute the Jacobian-vector product:
        // ----------------------
        // 𝛿 ← 𝜇⋅‖𝒚‖⁺,
        // 𝒔 ← 𝒙 + 𝛿⋅𝒚,
        // 𝒛 ← 𝓐(𝒔),
        // 𝒛 ← 𝛿⁺⋅𝒛 - 𝛿⁺⋅𝒘.
        // ----------------------
        const auto delta = safe_divide(mu, Blas::Norm2(y));
        Blas::Add(s_, x, y, delta);
        A.MatVec(z, s_);
        const real_t delta_recip = safe_divide(1.0, delta);
        Blas::Sub(z, z, delta_recip, w_, delta_recip);
      });
      solver->Solve(t_, r_, *op);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝒙 ← 𝒙 + 𝒕,
    // 𝒘 ← 𝓐(𝒙),
    // 𝒓 ← 𝒃 - 𝒘.
    // ----------------------
    Blas::AddAssign(x, t_);
    A.MatVec(w_, x);
    Blas::Sub(r_, b, w_);

    return Blas::Norm2(r_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Vector s_, t_, r_, w_;

}; // class JFNK

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
