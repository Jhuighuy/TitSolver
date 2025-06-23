/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/math.hpp"

#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c CG (Conjugate Gradients) linear self-adjoint definite operator
/// equation solver.
///
/// References:
/// @verbatim
/// [1] Hestenes, Magnus R. and Eduard Stiefel.
///     “Methods of conjugate gradients for solving linear systems.”
///     Journal of research of the National
///     Bureau of Standards 49 (1952): 409-435.
/// @endverbatim
template<class Mapping, class Preconditioner>
class CGSolver final : public IterativeSolver<Mapping, Preconditioner> {
public:

  using Base = IterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a @c CG solver.
  constexpr CGSolver(const Mapping& A, const Preconditioner& P) : Base{A, P} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    V.init(p_vec_, x_vec);
    V.init(r_vec_, b_vec);
    V.init(z_vec_, x_vec);

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
    A.residual(r_vec_, b_vec, x_vec);
    if (this->has_preconditioner()) {
      P.apply(z_vec_, r_vec_);
      V.copy(p_vec_, z_vec_);
      gamma_ = V.dot(r_vec_, z_vec_);
    } else {
      V.copy(p_vec_, r_vec_);
      gamma_ = V.dot(r_vec_, r_vec_);
    }

    return this->has_preconditioner() ? V.norm(r_vec_) : sqrt(gamma_);
  }

  constexpr auto iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& P = this->preconditioner();
    const auto& A = this->mapping();
    const auto& V = A.space();

    // Iterate:
    // ----------------------
    // 𝒛 ← 𝓐𝒑,
    // 𝛾̅ ← 𝛾,
    // 𝛼 ← 𝛾/<𝒑⋅𝒛>,
    // 𝒙 ← 𝒙 + 𝛼⋅𝒑,
    // 𝒓 ← 𝒓 - 𝛼⋅𝒛.
    // ----------------------
    A.apply(z_vec_, p_vec_);
    const auto gamma_bar = gamma_;
    const auto alpha     = safe_divide(gamma_, V.dot(p_vec_, z_vec_));
    V.add_assign(x_vec, p_vec_, alpha);
    V.sub_assign(r_vec_, z_vec_, alpha);

    // ----------------------
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒛 ← 𝓟𝒓,
    //   𝛾 ← <𝒓⋅𝒛>,
    // 𝗲𝗹𝘀𝗲:
    //   𝛾 ← <𝒓⋅𝒓>.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->has_preconditioner()) {
      P.apply(z_vec_, r_vec_);
      gamma_ = V.dot(r_vec_, z_vec_);
    } else {
      gamma_ = V.dot(r_vec_, r_vec_);
    }

    // ----------------------
    // 𝛽 ← 𝛾/𝛾̅,
    // 𝒑 ← (𝓟 ≠ 𝗻𝗼𝗻𝗲 ? 𝒛 : 𝒓) + 𝛽⋅𝒑.
    // ----------------------
    const auto beta = safe_divide(gamma_, gamma_bar);
    V.add(p_vec_, this->has_preconditioner() ? z_vec_ : r_vec_, p_vec_, beta);

    return this->has_preconditioner() ? V.norm(r_vec_) : sqrt(gamma_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num gamma_;
  mutable Vec p_vec_, r_vec_, z_vec_;

}; // class CgSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
