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

#include <memory>

#include "tit/core/basic_types.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"
#include "tit/ksp/precond.hpp"

namespace tit::ksp {

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief Abstract operator equation solver.
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike InVector, VectorLike OutVector = InVector>
class Solver : public Object {
public:

  /// @brief Solve the operator equation 𝓐(𝒙) = 𝒃.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  ///
  /// @returns Status of operation.
  virtual auto Solve(InVector& xVec,
                     const OutVector& bVec,
                     const Operator<InVector, OutVector>& anyOp) -> bool = 0;

}; // class Solver

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief Abstract operator equation iterative solver.
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike InVector, VectorLike OutVector = InVector>
class IterativeSolver : public Solver<InVector, OutVector> {
public:

  // NOLINTBEGIN(*-non-private-member-variables-in-classes)
  size_t Iteration{0};
  size_t NumIterations{2000};
  real_t AbsoluteError{0.0};
  real_t RelativeError{0.0};
  real_t AbsoluteTolerance{1.0e-6};
  real_t RelativeTolerance{1.0e-6};
  bool VerifySolution{false};
  PreconditionerSide PreSide{PreconditionerSide::Right};
  std::unique_ptr<Preconditioner<InVector>> PreOp{nullptr};
  // NOLINTEND(*-non-private-member-variables-in-classes)

protected:

  /// @brief Initialize the iterative solver.
  ///
  /// @param xVec Initial guess for the solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  virtual auto Init(const InVector& xVec,
                    const OutVector& bVec,
                    const Operator<InVector, OutVector>& anyOp,
                    const Preconditioner<InVector>* preOp) -> real_t = 0;

  /// @brief Iterate the solver.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm, ‖𝒃 - 𝓐(𝒙)‖.
  virtual auto Iterate(InVector& xVec,
                       const OutVector& bVec,
                       const Operator<InVector, OutVector>& anyOp,
                       const Preconditioner<InVector>* preOp) -> real_t = 0;

  /// @brief Finalize the iterations.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  virtual void Finalize(InVector& /*xVec*/,
                        const OutVector& /*bVec*/,
                        const Operator<InVector, OutVector>& /*anyOp*/,
                        const Preconditioner<InVector>* /*preOp*/) {}

public:

  auto Solve(InVector& xVec,
             const OutVector& bVec,
             const Operator<InVector, OutVector>& anyOp) -> bool override {
    // Initialize the solver.
    if (PreOp != nullptr) {
      PreOp->Build(xVec, bVec, anyOp);
    }
    const real_t initialError{
        (AbsoluteError = Init(xVec, bVec, anyOp, PreOp.get()))};
    if (AbsoluteTolerance > 0.0 && AbsoluteError < AbsoluteTolerance) {
      Finalize(xVec, bVec, anyOp, PreOp.get());
      return true;
    }

    // Iterate the solver:
    bool converged = false;
    for (Iteration = 0; !converged && (Iteration < NumIterations);
         ++Iteration) {
      AbsoluteError = Iterate(xVec, bVec, anyOp, PreOp.get());
      RelativeError = AbsoluteError / initialError;

      converged |=
          (AbsoluteTolerance > 0.0) && (AbsoluteError < AbsoluteTolerance);
      converged |=
          (RelativeTolerance > 0.0) && (RelativeError < RelativeTolerance);
    }

    // Exit the solver.
    Finalize(xVec, bVec, anyOp, PreOp.get());
    return converged;
  }

}; // class IterativeSolver

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief Abstract inner-outer iterative solver.
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike InVector, VectorLike OutVector = InVector>
class InnerOuterIterativeSolver : public IterativeSolver<InVector, OutVector> {
public:

  // NOLINTBEGIN(*-non-private-member-variables-in-classes)
  size_t InnerIteration{0};
  size_t NumInnerIterations{50};
  // NOLINTEND(*-non-private-member-variables-in-classes)

protected:

  /// @brief Initialize the outer iterations.
  ///
  /// This function is used invoked only once,
  ///   in the initialization phase.
  ///
  /// @param xVec Initial guess for the solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  virtual auto OuterInit(const InVector& xVec,
                         const OutVector& bVec,
                         const Operator<InVector, OutVector>& anyOp,
                         const Preconditioner<InVector>* preOp) -> real_t = 0;

  /// @brief Initialize the inner iterations.
  ///
  /// This function is invoked before the each inner iteration loop.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  virtual void InnerInit(const InVector& /*xVec*/,
                         const OutVector& /*bVec*/,
                         const Operator<InVector, OutVector>& /*anyOp*/,
                         const Preconditioner<InVector>* /*preOp*/) {}

  /// @brief Perform the inner iteration.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  ///
  /// @returns Residual norm, ‖𝒃 - 𝓐(𝒙)‖.
  virtual auto InnerIterate(InVector& xVec,
                            const OutVector& bVec,
                            const Operator<InVector, OutVector>& anyOp,
                            const Preconditioner<InVector>* preOp)
      -> real_t = 0;

  /// @brief Finalize the inner iterations.
  ///
  /// This function is called in order to finalize
  ///   the inner iterations or if some stopping criterion is met.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  virtual void InnerFinalize(InVector& /*xVec*/,
                             const OutVector& /*bVec*/,
                             const Operator<InVector, OutVector>& /*anyOp*/,
                             const Preconditioner<InVector>* /*preOp*/) {}

  /// @brief Finalize the outer iterations.
  ///
  /// This function is used invoked only once,
  ///   when some stopping criterion is met.
  ///
  /// @param xVec Solution vector, 𝒙.
  /// @param bVec Right-hand-side vector, 𝒃.
  /// @param anyOp Equation operator, 𝓐(𝒙).
  /// @param preOp Preconditioner operator, 𝓟(𝒙).
  virtual void OuterFinalize(InVector& /*xVec*/,
                             const OutVector& /*bVec*/,
                             const Operator<InVector, OutVector>& /*anyOp*/,
                             const Preconditioner<InVector>* /*preOp*/) {}

private:

  auto Init(const InVector& xVec,
            const OutVector& bVec,
            const Operator<InVector, OutVector>& anyOp,
            const Preconditioner<InVector>* preOp) -> real_t override {
    return OuterInit(xVec, bVec, anyOp, preOp);
  }

  auto Iterate(InVector& xVec,
               const OutVector& bVec,
               const Operator<InVector, OutVector>& anyOp,
               const Preconditioner<InVector>* preOp) -> real_t override {
    InnerIteration = this->Iteration % NumInnerIterations;
    if (InnerIteration == 0) {
      InnerInit(xVec, bVec, anyOp, preOp);
    }
    const real_t residualNorm{InnerIterate(xVec, bVec, anyOp, preOp)};
    if (InnerIteration == NumInnerIterations - 1) {
      InnerFinalize(xVec, bVec, anyOp, preOp);
    }
    return residualNorm;
  }

  void Finalize(InVector& xVec,
                const OutVector& bVec,
                const Operator<InVector, OutVector>& anyOp,
                const Preconditioner<InVector>* preOp) override {
    if (InnerIteration != NumInnerIterations - 1) {
      InnerFinalize(xVec, bVec, anyOp, preOp);
    }
    OuterFinalize(xVec, bVec, anyOp, preOp);
  }

}; // class InnerOuterIterativeSolver

/// ----------------------------------------------------------------- ///
/// @brief Solve an operator equation 𝓐(𝒙) = 𝒃,
///   when 𝓐(𝒙) is a non-uniform operator (𝓐(𝟢) ≠ 𝟢).
/// ----------------------------------------------------------------- ///
template<VectorLike Vector>
auto SolveNonUniform(Solver<Vector>& solver,
                     Vector& xVec,
                     const Vector& bVec,
                     const Operator<Vector>& anyOp) -> bool {
  Vector zVec;
  Vector fVec;

  zVec.Assign(xVec, false);
  fVec.Assign(bVec, false);

  // Solve an equation with the "uniformed" operator:
  // 𝓐(𝒙) - 𝓐(𝟢) = 𝒃 - 𝓐(𝟢).
  Blas::Fill(fVec, 0.0);
  anyOp.MatVec(zVec, fVec);
  Blas::Sub(fVec, bVec, zVec);

  const auto uniOp =
      MakeOperator<Vector>([&](Vector& yVec, const Vector& xxVec) {
        anyOp.MatVec(yVec, xxVec);
        Blas::SubAssign(yVec, zVec);
      });

  return solver.Solve(xVec, fVec, *uniOp);

} // SolveNonUniform

} // namespace tit::ksp
