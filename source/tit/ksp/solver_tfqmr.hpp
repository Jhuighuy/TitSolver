/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class for @c TFQMR and @c TFQMR1.
template<bool L1, class Mapping, class Preconditioner>
class BaseTFQMRSolver : public IterativeSolver<Mapping, Preconditioner> {
public:

  using Base = IterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    V.init(d_vec_, x_vec);
    V.init(r_tilde_vec_, x_vec);
    V.init(u_vec_, x_vec);
    V.init(v_vec_, x_vec);
    V.init(y_vec_, x_vec);
    V.init(s_vec_, x_vec);
    if (this->has_preconditioner()) V.init(z_vec_, x_vec);

    // Initialize:
    // ----------------------
    // 𝗶𝗳 𝘓₁:
    //   𝒅 ← 𝒙,
    // 𝗲𝗹𝘀𝗲:
    //   𝒅 ← {𝟢}ᵀ,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒚 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒚,
    //   𝒚 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒖 ← 𝒚,
    // 𝒓̃ ← 𝒖,
    // 𝜌 ← <𝒓̃⋅𝒓>, 𝜏 ← 𝜌¹ᐟ².
    // ----------------------
    if constexpr (L1) {
      V.copy(d_vec_, x_vec);
    } else {
      V.fill(d_vec_, Num{0});
    }
    A.residual(y_vec_, b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(z_vec_, y_vec_);
      P.apply(y_vec_, z_vec_);
    }
    V.copy(u_vec_, y_vec_);
    V.copy(r_tilde_vec_, u_vec_);
    rho_ = V.dot(r_tilde_vec_, u_vec_), tau_ = sqrt(rho_);

    return tau_;
  }

  constexpr auto iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    // Continue the iterations:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓟(𝒛 ← 𝓐𝒚),
    //   𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓐(𝒛 ← 𝓟𝒚),
    //   𝗲𝗹𝘀𝗲:
    //     𝒔 ← 𝓐𝒚.
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝒗 ← 𝒔,
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒖>,
    //   𝛽 ← 𝜌/𝜌̅,
    //   𝒗 ← 𝒔 + 𝛽⋅𝒗,
    //   𝒚 ← 𝒖 + 𝛽⋅𝒚,
    //   𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓟(𝒛 ← 𝓐𝒚),
    //   𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //     𝒔 ← 𝓐(𝒛 ← 𝓟𝒚),
    //   𝗲𝗹𝘀𝗲:
    //     𝒔 ← 𝓐𝒚,
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝒗 ← 𝒔 + 𝛽⋅𝒗.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->iteration() == 0) {
      if (this->has_left_preconditioner()) {
        P.chapply(s_vec_, z_vec_, A, y_vec_);
      } else if (this->has_right_preconditioner()) {
        A.chapply(s_vec_, z_vec_, P, y_vec_);
      } else {
        A.apply(s_vec_, y_vec_);
      }
      V.copy(v_vec_, s_vec_);
    } else {
      const auto rho_bar = rho_;
      rho_               = V.dot(r_tilde_vec_, u_vec_);
      const auto beta    = safe_divide(rho_, rho_bar);
      V.add(v_vec_, s_vec_, v_vec_, beta);
      V.add(y_vec_, u_vec_, y_vec_, beta);
      if (this->has_left_preconditioner()) {
        P.chapply(s_vec_, z_vec_, A, y_vec_);
      } else if (this->has_right_preconditioner()) {
        A.chapply(s_vec_, z_vec_, P, y_vec_);
      } else {
        A.apply(s_vec_, y_vec_);
      }
      V.add(v_vec_, s_vec_, v_vec_, beta);
    }

    // Update the solution:
    // ----------------------
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝗳𝗼𝗿 𝑚 = 𝟢, 𝟣 𝗱𝗼:
    //   𝒖 ← 𝒖 - 𝛼⋅𝒔,
    //   𝒅 ← 𝒅 + 𝛼⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒚),
    //   𝜔 ← ‖𝒖‖,
    //   𝗶𝗳 𝘓₁:
    //     𝗶𝗳 𝜔 < 𝜏:
    //       𝜏 ← 𝜔, 𝒙 ← 𝒅,
    //     𝗲𝗻𝗱 𝗶𝗳
    //   𝗲𝗹𝘀𝗲:
    //     𝑐𝑠, 𝑠𝑛 ← 𝘚𝘺𝘮𝘖𝘳𝘵𝘩𝘰(𝜏, 𝜔),
    //     𝜏 ← 𝑐𝑠⋅𝜔,
    //     𝒙 ← 𝒙 + 𝑐𝑠²⋅𝒅,
    //     𝒅 ← 𝑠𝑛²⋅𝒅,
    //   𝗲𝗻𝗱 𝗶𝗳
    //   𝗶𝗳 𝑚 = 𝟢:
    //     𝒚 ← 𝒚 - 𝛼⋅𝒗,
    //     𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //       𝒔 ← 𝓟(𝒛 ← 𝓐𝒚).
    //     𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //       𝒔 ← 𝓐(𝒛 ← 𝓟𝒚).
    //     𝗲𝗹𝘀𝗲:
    //       𝒔 ← 𝓐𝒚.
    //     𝗲𝗻𝗱 𝗶𝗳
    //   𝗲𝗻𝗱 𝗶𝗳
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    const auto alpha = safe_divide(rho_, V.dot(r_tilde_vec_, v_vec_));
    for (size_t m = 0; m <= 1; ++m) {
      V.sub_assign(u_vec_, s_vec_, alpha);
      V.add_assign(d_vec_,
                   this->has_right_preconditioner() ? z_vec_ : y_vec_,
                   alpha);
      const auto omega = V.norm(u_vec_);
      if constexpr (L1) {
        if (omega < tau_) {
          tau_ = omega, V.copy(x_vec, d_vec_);
        }
      } else {
        const auto [cs, sn, rr] = sym_ortho(tau_, omega);
        tau_                    = omega * cs;
        V.add_assign(x_vec, d_vec_, pow2(cs));
        V.scale_assign(d_vec_, pow2(sn));
      }
      if (m == 0) {
        V.sub_assign(y_vec_, v_vec_, alpha);
        if (this->has_left_preconditioner()) {
          P.chapply(s_vec_, z_vec_, A, y_vec_);
        } else if (this->has_right_preconditioner()) {
          A.chapply(s_vec_, z_vec_, P, y_vec_);
        } else {
          A.apply(s_vec_, y_vec_);
        }
      }
    }

    // Compute the residual norm
    // (or it's upper bound estimate in the ℒ₂ case):
    // ----------------------
    // 𝜏̃ ← 𝜏,
    // 𝗶𝗳 𝗻𝗼𝘁 𝘓₁:
    //   𝜏̃ ← 𝜏⋅(𝟤𝑘 + 𝟥)¹ᐟ².
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    auto tau_tilde = tau_;
    if constexpr (!L1) {
      const size_t k = this->iteration();
      tau_tilde *= sqrt(Num{2} * static_cast<Num>(k) + Num{3});
    }

    return tau_tilde;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

protected:

  /// Construct a @c TFQMR(1) solver.
  constexpr BaseTFQMRSolver(const Mapping& A, const Preconditioner& P)
      : Base{A, P} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num rho_, tau_;
  mutable Vec d_vec_, r_tilde_vec_, u_vec_, v_vec_, y_vec_, s_vec_, z_vec_;

}; // class BaseTFQMRSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c TFQMR (Transpose-Free Quasi-Minimal Residual) linear operator
/// equation solver.
///
/// @c TFQMR, like the other @c BiCG type methods, normally requires two
/// operator-vector products per iteration. But, unlike the other @c BiCG type
/// methods, @c TFQMR does not implicitly contain the residual norm estimate,
/// only the rough upper bound is avariable, so at the latter iterations an
/// extra operator-vector product per iteration may be required for the explicit
/// residual estimation.
///
/// @c TFQMR typically converges much smoother, than @c CGS and @c BiCGStab.
///
/// References:
/// @verbatim
/// [1] Freund, Roland W.
///     “A Transpose-Free Quasi-Minimal Residual Algorithm
///      for Non-Hermitian Linear Systems.”
///     SIAM J. Sci. Comput. 14 (1993): 470-482.
/// [2] Freund, Roland W.
///     “Transpose-Free Quasi-Minimal Residual Methods
///      for Non-Hermitian Linear Systems.” (1994).
/// @endverbatim
template<class Mapping, class Preconditioner>
class TFQMRSolver final :
    public BaseTFQMRSolver</*L1=*/false, Mapping, Preconditioner> {
public:

  /// Construct a @c TFQMR solver.
  constexpr TFQMRSolver(const Mapping& A, const Preconditioner& P)
      : BaseTFQMRSolver</*L1=*/false, Mapping, Preconditioner>{A, P} {}

}; // class TFQMRSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c TFQMR1 (Transpose-Free 1-norm Quasi-Minimal Residual) linear operator
/// equation solver.
///
/// @c TFQMR1, like the other @c BiCG type solvers, requires two operator-vector
/// products per iteration. Unlike @c TFQMR, @c TFQMR1 implicitly contains the
/// residual norm estimate, so no extra operator-vector products are required.
///
/// @c TFQMR1 typically converges much smoother, than @c CGS and @c BiCGStab and
/// is slightly faster than @c TFQMR.
///
/// References:
/// @verbatim
/// [1] H.M Bücker,
///     “A Transpose-Free 1-norm Quasi-Minimal Residual Algorithm
///      for Non-Hermitian Linear Systems.“, FZJ-ZAM-IB-9706.
/// @endverbatim
template<class Mapping, class Preconditioner>
class TFQMR1Solver final :
    public BaseTFQMRSolver</*L1=*/true, Mapping, Preconditioner> {
public:

  /// Construct a @c TFQMR solver.
  constexpr TFQMR1Solver(const Mapping& A, const Preconditioner& P)
      : BaseTFQMRSolver</*L1=*/true, Mapping, Preconditioner>{A, P} {}

}; // class TFQMR1Solver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
