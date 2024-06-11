/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>

#include "tit/core/basic_types.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract operator equation solver.
template<blas::vector InVector, blas::vector OutVector = InVector>
class Solver : public Object {
public:

  /// @brief Solve the operator equation 𝓐(𝒙) = 𝒃.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  ///
  /// @returns Status of operation.
  constexpr virtual auto solve(InVector& x,
                               const OutVector& b,
                               const Operator<InVector, OutVector>& A)
      -> bool = 0;

}; // class Solver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract operator equation iterative solver.
template<blas::vector InVector, blas::vector OutVector = InVector>
class IterativeSolver : public Solver<InVector, OutVector> {
public:

  // NOLINTBEGIN(*-non-private-member-variables-in-classes)
  size_t Iteration = 0;
  size_t NumIterations = 2000;
  PreconditionerSide PreSide = PreconditionerSide::Right;
  std::unique_ptr<Preconditioner<InVector>> PreOp;
  // NOLINTEND(*-non-private-member-variables-in-classes)

protected:

  /// Initialize the iterative solver.
  ///
  /// @param x Initial guess for the solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr virtual auto init(const InVector& x,
                              const OutVector& b,
                              const Operator<InVector, OutVector>& A,
                              const Preconditioner<InVector>* P) -> real_t = 0;

  /// Iterate the solver.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr virtual auto iter(InVector& x,
                              const OutVector& b,
                              const Operator<InVector, OutVector>& A,
                              const Preconditioner<InVector>* P) -> real_t = 0;

  /// Finalize the iterations.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr virtual void finalize(InVector& /*x*/,
                                  const OutVector& /*b*/,
                                  const Operator<InVector, OutVector>& /*A*/,
                                  const Preconditioner<InVector>* /*P*/) {}

public:

  constexpr auto solve(InVector& x,
                       const OutVector& b,
                       const Operator<InVector, OutVector>& A)
      -> bool override {
    // Initialize the solver.
    if (PreOp != nullptr) {
      PreOp->Build(x, b, A);
    }
    const real_t init_error = (abs_error_ = init(x, b, A, PreOp.get()));
    if (abs_error_tol_ > 0.0 && abs_error_ < abs_error_tol_) {
      finalize(x, b, A, PreOp.get());
      return true;
    }

    // Iterate the solver:
    bool converged = false;
    for (Iteration = 0; !converged && (Iteration < NumIterations);
         ++Iteration) {
      abs_error_ = iter(x, b, A, PreOp.get());
      rel_error_ = abs_error_ / init_error;
      converged |= (abs_error_tol_ > 0.0) && //
                   (abs_error_ < abs_error_tol_);
      converged |= (rel_error_tol_ > 0.0) && //
                   (rel_error_ < rel_error_tol_);
    }

    // Exit the solver.
    finalize(x, b, A, PreOp.get());
    return converged;
  }

private:

  real_t abs_error_ = 0.0;
  real_t rel_error_ = 0.0;
  real_t abs_error_tol_ = 1.0e-6;
  real_t rel_error_tol_ = 1.0e-6;
  bool verify_solution_ = false;

}; // class IterativeSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract inner-outer iterative solver.
template<blas::vector InVector, blas::vector OutVector = InVector>
class InnerOuterIterativeSolver : public IterativeSolver<InVector, OutVector> {
public:

  // NOLINTBEGIN(*-non-private-member-variables-in-classes)
  size_t InnerIteration = 0;
  size_t NumInnerIterations = 50;
  // NOLINTEND(*-non-private-member-variables-in-classes)

protected:

  /// Initialize the outer iterations.
  ///
  /// This function is used invoked only once, in the initialization phase.
  ///
  /// @param x Initial guess for the solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr virtual auto outer_init(const InVector& x,
                                    const OutVector& b,
                                    const Operator<InVector, OutVector>& A,
                                    const Preconditioner<InVector>* P)
      -> real_t = 0;

  /// Initialize the inner iterations.
  ///
  /// This function is invoked before the each inner iteration loop.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr virtual void inner_init(const InVector& /*x*/,
                                    const OutVector& /*b*/,
                                    const Operator<InVector, OutVector>& /*A*/,
                                    const Preconditioner<InVector>* /*P*/) {}

  /// Perform the inner iteration.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr virtual auto inner_iter(InVector& x,
                                    const OutVector& b,
                                    const Operator<InVector, OutVector>& A,
                                    const Preconditioner<InVector>* P)
      -> real_t = 0;

  /// Finalize the inner iterations.
  ///
  /// This function is called in order to finalize the inner iterations or if
  /// some stopping criterion is met.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr virtual void inner_finalize(
      InVector& /*x*/,
      const OutVector& /*b*/,
      const Operator<InVector, OutVector>& /*A*/,
      const Preconditioner<InVector>* /*P*/) {}

  /// Finalize the outer iterations.
  ///
  /// This function is used invoked only once, when some stopping criterion is
  /// met.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr virtual void outer_finalize(
      InVector& /*x*/,
      const OutVector& /*b*/,
      const Operator<InVector, OutVector>& /*A*/,
      const Preconditioner<InVector>* /*P*/) {}

private:

  constexpr auto init(const InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t override {
    return outer_init(x, b, A, P);
  }

  constexpr auto iter(InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t override {
    InnerIteration = this->Iteration % NumInnerIterations;
    if (InnerIteration == 0) {
      inner_init(x, b, A, P);
    }
    const real_t residual_norm = inner_iter(x, b, A, P);
    if (InnerIteration == NumInnerIterations - 1) {
      inner_finalize(x, b, A, P);
    }
    return residual_norm;
  }

  void finalize(InVector& x,
                const OutVector& b,
                const Operator<InVector, OutVector>& A,
                const Preconditioner<InVector>* P) override {
    if (InnerIteration != NumInnerIterations - 1) {
      inner_finalize(x, b, A, P);
    }
    outer_finalize(x, b, A, P);
  }

}; // class InnerOuterIterativeSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<blas::vector Vector, class Op>
  requires std::invocable<const Op&, Vector&, const Vector&>
constexpr auto solve(Solver<Vector>& solver,
                     const Op& A,
                     Vector& x,
                     const Vector& b) -> bool {
  return solver.solve(x, b, *MakeOperator<Vector>(A));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Solve an operator equation 𝓐(𝒙) = 𝒃, when 𝓐(𝒙) is a non-uniform
/// operator (𝓐(𝟢) ≠ 𝟢).
template<blas::vector Vector>
constexpr auto solve_nonuniform(Solver<Vector>& solver,
                                const Operator<Vector>& A,
                                Vector& x,
                                const Vector& b) -> bool {
  Vector z;
  Vector f;

  z.Assign(x, false);
  f.Assign(b, false);

  // Solve an equation with the "uniformed" operator:
  // 𝓐(𝒙) - 𝓐(𝟢) = 𝒃 - 𝓐(𝟢).
  Blas::Fill(f, 0.0);
  A.MatVec(z, f);
  Blas::Sub(f, b, z);

  const auto U = MakeOperator<Vector>([&](Vector& y, const Vector& xx) {
    A.MatVec(y, xx);
    Blas::SubAssign(y, z);
  });

  return solver.solve(x, f, *U);
}

template<blas::vector Vector, class Op>
  requires std::invocable<const Op&, Vector&, const Vector&>
constexpr auto solve_nonuniform(Solver<Vector>& solver,
                                const Op& A,
                                Vector& x,
                                const Vector& b) -> bool {
  return solve_nonuniform(solver, *MakeOperator<Vector>(A), x, b);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
