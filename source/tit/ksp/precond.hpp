/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/operator.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Preconditioner side.
enum class PreconditionerSide : uint8_t {
  /// Left preconditioned equation is solved, 𝓟𝓐𝒙 = 𝓟𝒃.
  ///
  /// When the left preconditioning is used, iterative solver tracks convergence
  /// by the left preconditioned residual norm, ‖𝓟(𝒃 - 𝓐𝒙)‖.
  Left,

  /// Right preconditioned equation is solved, 𝓐𝓟𝒙̃ = 𝒃, 𝓟𝒙̃ = 𝒙.
  ///
  /// When the right preconditioning is used, iterative solver tracks
  /// convergence by the unpreconditioned residual norm, ‖𝒃 - 𝓐𝒙‖.
  Right,

  /// Symmetric preconditioned equation is solved,
  ///   𝓜𝓐𝓝𝒙̃ = 𝓜𝒃, 𝓝𝒙̃ = 𝒙, 𝓟 = 𝓜𝓝.
  ///
  /// When the symmetric preconditioning is used, iterative solver tracks
  /// convergence by the partially preconditioned residual norm, ‖𝓜(𝒃 - 𝓐𝒙)‖.
  Symmetric,

}; // enum class PreconditionerSide

/// Abstract preconditioner operator.
template<blas::vector Vector>
class Preconditioner : public Operator<Vector> {
public:

  /// Build the preconditioner.
  ///
  /// @param x Solution vector, 𝒙.
  /// @param b Right-hand-side vector, 𝒃.
  /// @param A Operator to build the preconditioner upon.
  constexpr virtual void Build(const Vector& /*x*/,
                               const Vector& /*b*/,
                               const Operator<Vector>& /*A*/) {}

}; // class Preconditioner

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Identity preconditioner, intended to be used for debugging only.
template<blas::vector Vector>
class IdentityPreconditioner final : public Preconditioner<Vector> {
private:

  constexpr void MatVec(Vector& y, const Vector& x) const override {
    Blas::Set(y, x);
  }

  constexpr void ConjMatVec(Vector& x, const Vector& y) const override {
    Blas::Set(x, y);
  }

}; // class IdentityPreconditioner

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
