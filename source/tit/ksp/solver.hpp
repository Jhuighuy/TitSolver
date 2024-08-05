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
  constexpr auto solve(InVector& x,
                       const OutVector& b,
                       const Operator<InVector, OutVector>& A) -> bool;

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
  constexpr auto init(const InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t;

  /// Iterate the solver.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr auto iter(InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t;

  /// Finalize the iterations.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr void finalize(InVector& /*x*/,
                          const OutVector& /*b*/,
                          const Operator<InVector, OutVector>& /*A*/,
                          const Preconditioner<InVector>* /*P*/) {}

public:

  constexpr auto solve(this auto& self,
                       InVector& x,
                       const OutVector& b,
                       const Operator<InVector, OutVector>& A) -> bool {
    // Initialize the solver.
    if (self.PreOp != nullptr) {
      self.PreOp->Build(x, b, A);
    }
    const auto init_error =
        (self.abs_error_ = self.init(x, b, A, self.PreOp.get()));
    if (self.abs_error_tol_ > 0.0 && self.abs_error_ < self.abs_error_tol_) {
      self.finalize(x, b, A, self.PreOp.get());
      return true;
    }

    // Iterate the solver.
    bool converged = false;
    for (self.Iteration = 0;
         !converged && (self.Iteration < self.NumIterations);
         ++self.Iteration) {
      self.abs_error_ = self.iter(x, b, A, self.PreOp.get());
      self.rel_error_ = self.abs_error_ / init_error;
      converged |= (self.abs_error_tol_ > 0.0) && //
                   (self.abs_error_ < self.abs_error_tol_);
      converged |= (self.rel_error_tol_ > 0.0) && //
                   (self.rel_error_ < self.rel_error_tol_);
    }

    // Exit the solver.
    self.finalize(x, b, A, self.PreOp.get());
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
  constexpr auto outer_init(const InVector& x,
                            const OutVector& b,
                            const Operator<InVector, OutVector>& A,
                            const Preconditioner<InVector>* P) -> real_t;

  /// Initialize the inner iterations.
  ///
  /// This function is invoked before the each inner iteration loop.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr void inner_init(const InVector& /*x*/,
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
  constexpr auto inner_iter(InVector& x,
                            const OutVector& b,
                            const Operator<InVector, OutVector>& A,
                            const Preconditioner<InVector>* P) -> real_t;

  /// Finalize the inner iterations.
  ///
  /// This function is called in order to finalize the inner iterations or if
  /// some stopping criterion is met.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Equation operator, 𝓐(𝒙).
  /// @param P Preconditioner operator, 𝓟(𝒙).
  constexpr void inner_finalize(InVector& /*x*/,
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
  constexpr void outer_finalize(InVector& /*x*/,
                                const OutVector& /*b*/,
                                const Operator<InVector, OutVector>& /*A*/,
                                const Preconditioner<InVector>* /*P*/) {}

private:

  constexpr auto init(this auto& self,
                      const InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t {
    return self.outer_init(x, b, A, P);
  }

  constexpr auto iter(this auto& self,
                      InVector& x,
                      const OutVector& b,
                      const Operator<InVector, OutVector>& A,
                      const Preconditioner<InVector>* P) -> real_t {
    self.InnerIteration = self.Iteration % self.NumInnerIterations;
    if (self.InnerIteration == 0) {
      self.inner_init(x, b, A, P);
    }
    const auto residual_norm = self.inner_iter(x, b, A, P);
    if (self.InnerIteration == self.NumInnerIterations - 1) {
      self.inner_finalize(x, b, A, P);
    }
    return residual_norm;
  }

  void finalize(this auto& self,
                InVector& x,
                const OutVector& b,
                const Operator<InVector, OutVector>& A,
                const Preconditioner<InVector>* P) {
    if (self.InnerIteration != self.NumInnerIterations - 1) {
      self.inner_finalize(x, b, A, P);
    }
    self.outer_finalize(x, b, A, P);
  }

  friend class IterativeSolver<InVector, OutVector>;

}; // class InnerOuterIterativeSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<blas::vector Vector, class Op>
  requires std::invocable<const Op&, Vector&, const Vector&>
constexpr auto solve(std::derived_from<Solver<Vector>> auto& solver,
                     const Op& A,
                     Vector& x,
                     const Vector& b) -> bool {
  return solver.solve(x, b, *MakeOperator<Vector>(A));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Solve an operator equation 𝓐(𝒙) = 𝒃, when 𝓐(𝒙) is a non-uniform
/// operator (𝓐(𝟢) ≠ 𝟢).
template<blas::vector Vector>
constexpr auto solve_nonuniform(std::derived_from<Solver<Vector>> auto& solver,
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
