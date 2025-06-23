/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/math.hpp"
#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c CGS (Conjugate Gradients Squared) linear operator equation solver.
///
/// @c CGS convergence behavior may be very erratic.
///
/// @c CGS, like the other @c BiCG type solvers, requires two operator
/// multiplications per iteration.
///
/// References:
/// @verbatim
/// [1] Sonneveld, Peter.
///     “CGS, A Fast Lanczos-Type Solver for Nonsymmetric Linear systems.”
///     SIAM J. Sci. Stat. Comput., 10:36-52, 1989.
/// @endverbatim
template<class Mapping, class Preconditioner>
class CGSSolver final : public IterativeSolver<Mapping, Preconditioner> {
public:

  using Base = IterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a @c CGS solver.
  constexpr CGSSolver(const Mapping& A, const Preconditioner& P) : Base{A, P} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    V.init(p_vec_, x_vec);
    V.init(q_vec_, x_vec);
    V.init(r_vec_, x_vec);
    V.init(r_tilde_vec_, x_vec);
    V.init(u_vec_, x_vec);
    V.init(v_vec_, x_vec);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒖 ← 𝒓,
    //   𝒓 ← 𝓟𝒖,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓,
    // 𝜌 ← <𝒓̃⋅𝒓>.
    // ----------------------
    A.residual(r_vec_, b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(u_vec_, r_vec_);
      P.apply(r_vec_, u_vec_);
    }
    V.copy(r_tilde_vec_, r_vec_);
    rho_ = V.dot(r_tilde_vec_, r_vec_);

    return sqrt(rho_);
  }

  constexpr auto iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& P = this->preconditioner();
    const auto& A = this->mapping();
    const auto& V = A.space();

    // Continue the iterations:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒖 ← 𝒓,
    //   𝒑 ← 𝒖.
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓>,
    //   𝛽 ← 𝜌/𝜌̅,
    //   𝒖 ← 𝒓 + 𝛽⋅𝒒,
    //   𝒑 ← 𝒒 + 𝛽⋅𝒑,
    //   𝒑 ← 𝒖 + 𝛽⋅𝒑.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->iteration() == 0) {
      V.copy(u_vec_, r_vec_);
      V.copy(p_vec_, u_vec_);
    } else {
      const auto rho_bar = rho_;
      rho_               = V.dot(r_tilde_vec_, r_vec_);
      const auto beta    = safe_divide(rho_, rho_bar);
      V.add(u_vec_, r_vec_, q_vec_, beta);
      V.add(p_vec_, q_vec_, p_vec_, beta);
      V.add(p_vec_, u_vec_, p_vec_, beta);
    }

    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓟(𝒒 ← 𝓐𝒑),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒒 ← 𝓟𝒑),
    // 𝗲𝗹𝘀𝗲:
    //   𝒗 ← 𝓐𝒑,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝒒 ← 𝒖 - 𝛼⋅𝒗,
    // 𝒗 ← 𝒖 + 𝒒.
    // ----------------------
    if (this->has_left_preconditioner()) {
      P.chapply(v_vec_, q_vec_, A, p_vec_);
    } else if (this->has_right_preconditioner()) {
      A.chapply(v_vec_, q_vec_, P, p_vec_);
    } else {
      A.apply(v_vec_, p_vec_);
    }
    const auto alpha = safe_divide(rho_, V.dot(r_tilde_vec_, v_vec_));
    V.sub(q_vec_, u_vec_, v_vec_, alpha);
    V.add(v_vec_, u_vec_, q_vec_);

    // Update the solution and the residual:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒙 ← 𝒙 + 𝛼⋅𝒗,
    //   𝒗 ← 𝓟(𝒖 ← 𝓐𝒗),
    //   𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒖 ← 𝓟𝒗),
    //   𝒙 ← 𝒙 + 𝛼⋅𝒖,
    //   𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // 𝗲𝗹𝘀𝗲:
    //   𝒖 ← 𝓐𝒗,
    //   𝒙 ← 𝒙 + 𝛼⋅𝒗,
    //   𝒓 ← 𝒓 - 𝛼⋅𝒖.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->has_left_preconditioner()) {
      V.add_assign(x_vec, v_vec_, alpha);
      P.chapply(v_vec_, u_vec_, A, v_vec_);
      V.sub_assign(r_vec_, v_vec_, alpha);
    } else if (this->has_right_preconditioner()) {
      A.chapply(v_vec_, u_vec_, P, v_vec_);
      V.add_assign(x_vec, u_vec_, alpha);
      V.sub_assign(r_vec_, v_vec_, alpha);
    } else {
      A.apply(u_vec_, v_vec_);
      V.add_assign(x_vec, v_vec_, alpha);
      V.sub_assign(r_vec_, u_vec_, alpha);
    }

    return V.norm(r_vec_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num rho_;
  mutable Vec p_vec_, q_vec_, r_vec_, r_tilde_vec_, u_vec_, v_vec_;

}; // class CGSSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
