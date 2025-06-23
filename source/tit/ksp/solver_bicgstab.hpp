/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c BiCGStab (Biconjugate Gradients Stabilized) linear operator equation
/// solver.
///
/// @c BiCGStab, like the other @c BiCG type solvers, requires
/// two operator multiplications per iteration.
///
/// @c BiCGStab typically converges much smoother, than @c CGS.
///
/// References:
/// @verbatim
/// [1] Henk A. van der Vorst.
///     “Bi-CGSTAB: A Fast and Smoothly Converging Variant of Bi-CG
///      for the Solution of Nonsymmetric Linear Systems.”
///     SIAM J. Sci. Comput. 13 (1992): 631-644.
/// @endverbatim
template<class Mapping, class Preconditioner>
class BiCGStabSolver final : public IterativeSolver<Mapping, Preconditioner> {
public:

  using Base = IterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a @c BiCGStab solver.
  constexpr BiCGStabSolver(const Mapping& A, const Preconditioner& P)
      : Base{A, P} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();

    V.init(p_vec_, x_vec);
    V.init(r_vec_, x_vec);
    V.init(r_tilde_vec_, x_vec);
    V.init(t_vec_, x_vec);
    V.init(v_vec_, x_vec);
    if (this->has_preconditioner()) V.init(z_vec_, x_vec);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒓,
    //   𝒓 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓,
    // 𝜌 ← <𝒓̃⋅𝒓>.
    // ----------------------
    A.residual(r_vec_, b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(z_vec_, r_vec_);
      A.apply(r_vec_, z_vec_);
    }
    V.copy(r_tilde_vec_, r_vec_);
    rho_ = V.dot(r_tilde_vec_, r_vec_);

    return sqrt(rho_);
  }

  constexpr auto iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    // Continue the iterations:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒑 ← 𝒓.
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓>,
    //   𝛽 ← (𝜌/𝜌̅)⋅(𝛼/𝜔),
    //   𝒑 ← 𝒑 - 𝜔⋅𝒗,
    //   𝒑 ← 𝒓 + 𝛽⋅𝒑.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->iteration() == 0) {
      V.copy(p_vec_, r_vec_);
    } else {
      const auto rho_bar = rho_;
      rho_               = V.dot(r_tilde_vec_, r_vec_);
      const auto beta    = safe_divide(alpha_ * rho_, omega_ * rho_bar);
      V.sub_assign(p_vec_, v_vec_, omega_);
      V.add(p_vec_, r_vec_, p_vec_, beta);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓟(𝒛 ← 𝓐𝒑),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒗 ← 𝓐(𝒛 ← 𝓟𝒑),
    // 𝗲𝗹𝘀𝗲:
    //   𝒗 ← 𝓐𝒑,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒗>,
    // 𝒙 ← 𝒙 + 𝛼⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒑),
    // 𝒓 ← 𝒓 - 𝛼⋅𝒗.
    // ----------------------
    if (this->has_left_preconditioner()) {
      P.chapply(v_vec_, z_vec_, A, p_vec_);
    } else if (this->has_right_preconditioner()) {
      A.chapply(v_vec_, z_vec_, P, p_vec_);
    } else {
      A.apply(v_vec_, p_vec_);
    }
    alpha_ = safe_divide(rho_, V.dot(r_tilde_vec_, v_vec_));
    V.add_assign(x_vec,
                 this->has_right_preconditioner() ? z_vec_ : p_vec_,
                 alpha_);
    V.sub_assign(r_vec_, v_vec_, alpha_);

    // Update the solution and the residual again:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒕 ← 𝓟(𝒛 ← 𝓐𝒓),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒕 ← 𝓐(𝒛 ← 𝓟𝒓),
    // 𝗲𝗹𝘀𝗲:
    //   𝒕 ← 𝓐𝒓,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝜔 ← <𝒕⋅𝒓>/<𝒕⋅𝒕>,
    // 𝒙 ← 𝒙 + 𝜔⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒓),
    // 𝒓 ← 𝒓 - 𝜔⋅𝒕.
    // ----------------------
    if (this->has_left_preconditioner()) {
      P.chapply(t_vec_, z_vec_, A, r_vec_);
    } else if (this->has_right_preconditioner()) {
      A.chapply(t_vec_, z_vec_, P, r_vec_);
    } else {
      A.apply(t_vec_, r_vec_);
    }
    omega_ = safe_divide(V.dot(t_vec_, r_vec_), V.dot(t_vec_, t_vec_));
    V.add_assign(x_vec,
                 this->has_right_preconditioner() ? z_vec_ : r_vec_,
                 omega_);
    V.sub_assign(r_vec_, t_vec_, omega_);

    return V.norm(r_vec_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num alpha_, rho_, omega_;
  mutable Vec p_vec_, r_vec_, r_tilde_vec_, t_vec_, v_vec_, z_vec_;

}; // class BiCGStabSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c BiCGStab(l) (Biconjugate Gradients Stabilized) linear operator
/// equation solver.
///
/// @c BiCGStab(l), like the other @c BiCG type solvers, requires
///   two operator multiplications per iteration.
///
/// References:
/// @verbatim
/// [1] Gerard L. G. Sleijpen and Diederik R. Fokkema.
///     “BiCGStab(l) for Linear Equations involving
///      Unsymmetric Matrices with Complex Spectrum.”
///     Electronic Transactions on Numerical Analysis 1 (1993): 11-32.
/// @endverbatim
template<class Mapping, class Preconditioner>
class BiCGStabLSolver final :
    public InnerOuterIterativeSolver<Mapping, Preconditioner> {
public:

  using Base = InnerOuterIterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a @c BiCGStab(l) solver.
  constexpr BiCGStabLSolver(const Mapping& A, const Preconditioner& P)
      : Base{A, P} {
    // Let's tweak some defaults.
    this->set_num_inner_iterations(2);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  l = this->num_inner_iterations();

    gamma_.resize(l + 1);
    gamma_bar_.resize(l + 1);
    gamma_bar_bar_.resize(l + 1);
    sigma_.resize(l + 1);
    tau_.assign(l + 1, l + 1);

    V.init(r_tilde_vec_, x_vec);
    if (this->has_preconditioner()) V.init(z_vec_, x_vec);

    r_vecs_.resize(l + 1);
    for (auto& r_vec : r_vecs_) V.init(r_vec, x_vec);
    u_vecs_.resize(l + 1);
    for (auto& u_vec : u_vecs_) V.init(u_vec, x_vec);

    // Initialize:
    // ----------------------
    // 𝒖₀ ← {𝟢}ᵀ,
    // 𝒓₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒛 ← 𝒓₀,
    //   𝒓₀ ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒓̃ ← 𝒓₀,
    // 𝜌 ← <𝒓̃⋅𝒓₀>.
    // ----------------------
    V.fill(u_vecs_[0], 0.0);
    A.residual(r_vecs_[0], b_vec, x_vec);
    if (this->has_preconditioner()) {
      V.swap(z_vec_, r_vecs_[0]);
      P.apply(r_vecs_[0], z_vec_);
    }
    V.copy(r_tilde_vec_, r_vecs_[0]);
    rho_ = V.dot(r_tilde_vec_, r_vecs_[0]);

    return sqrt(rho_);
  }

  constexpr auto inner_iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  l = this->num_inner_iterations();
    const auto  j = this->inner_iteration();

    // BiCG part:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝒖₀ ← 𝒓₀,
    // 𝗲𝗹𝘀𝗲:
    //   𝜌̅ ← 𝜌,
    //   𝜌 ← <𝒓̃⋅𝒓ⱼ>,
    //   𝛽 ← 𝛼⋅𝜌/𝜌̅,
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑗 𝗱𝗼:
    //     𝒖ᵢ ← 𝒓ᵢ - 𝛽⋅𝒖ᵢ,
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒖ⱼ₊₁ ← 𝓐𝒖ⱼ,
    // 𝗲𝗹𝘀𝗲:
    //   𝒖ⱼ₊₁ ← 𝓟(𝒛 ← 𝓐𝒖ⱼ),
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛼 ← 𝜌/<𝒓̃⋅𝒖ⱼ₊₁>,
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑗 𝗱𝗼:
    //   𝒓ᵢ ← 𝒓ᵢ - 𝛼⋅𝒖ᵢ₊₁.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    if (this->iteration() == 0) {
      V.copy(u_vecs_[0], r_vecs_[0]);
    } else {
      const auto rho_bar = rho_;
      rho_               = V.dot(r_tilde_vec_, r_vecs_[j]);
      const auto beta    = safe_divide(alpha_ * rho_, rho_bar);
      for (size_t i = 0; i <= j; ++i) {
        V.sub(u_vecs_[i], r_vecs_[i], u_vecs_[i], beta);
      }
    }
    if (this->has_preconditioner()) {
      P.chapply(u_vecs_[j + 1], z_vec_, A, u_vecs_[j]);
    } else {
      A.apply(u_vecs_[j + 1], u_vecs_[j]);
    }
    alpha_ = safe_divide(rho_, V.dot(r_tilde_vec_, u_vecs_[j + 1]));
    for (size_t i = 0; i <= j; ++i) {
      V.sub_assign(r_vecs_[i], u_vecs_[i + 1], alpha_);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝒙 ← 𝒙 + 𝛼⋅𝒖₀,
    // 𝗶𝗳 𝓟 ≠ 𝗻𝗼𝗻𝗲:
    //   𝒓ⱼ₊₁ ← 𝓟(𝒛 ← 𝓐𝒓ⱼ).
    // 𝗲𝗹𝘀𝗲:
    //   𝒓ⱼ₊₁ ← 𝓐𝒓ⱼ.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    V.add_assign(x_vec, u_vecs_[0], alpha_);
    if (this->has_preconditioner()) {
      P.chapply(r_vecs_[j + 1], z_vec_, A, r_vecs_[j]);
    } else {
      A.apply(r_vecs_[j + 1], r_vecs_[j]);
    }

    if (j == l - 1) {
      // Minimal residual part:
      // ----------------------
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 𝗱𝗼:
      //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑗 - 𝟣 𝗱𝗼:
      //     𝜏ᵢⱼ ← <𝒓ᵢ⋅𝒓ⱼ>/𝜎ᵢ,
      //     𝒓ⱼ ← 𝒓ⱼ - 𝜏ᵢⱼ⋅𝒓ᵢ,
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      //   𝜎ⱼ ← <𝒓ⱼ⋅𝒓ⱼ>,
      //   𝛾̅ⱼ ← <𝒓₀⋅𝒓ⱼ>/𝜎ⱼ,
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      for (size_t j = 1; j <= l; ++j) {
        for (size_t i = 1; i < j; ++i) {
          tau_[i, j] = safe_divide(V.dot(r_vecs_[i], r_vecs_[j]), sigma_[i]);
          V.sub_assign(r_vecs_[j], r_vecs_[i], tau_[i, j]);
        }
        sigma_[j]     = V.dot(r_vecs_[j], r_vecs_[j]);
        gamma_bar_[j] = safe_divide(V.dot(r_vecs_[0], r_vecs_[j]), sigma_[j]);
      }

      // ----------------------
      // 𝜔 ← 𝛾ₗ ← 𝛾̅ₗ, 𝜌 ← -𝜔⋅𝜌,
      // 𝗳𝗼𝗿 𝑗 = 𝑙 - 𝟣, 𝟣, -𝟣 𝗱𝗼:
      //   𝛾ⱼ ← 𝛾̅ⱼ,
      //   𝗳𝗼𝗿 𝑖 = 𝑗 + 𝟣, 𝑙 𝗱𝗼:
      //     𝛾ⱼ ← 𝛾ⱼ - 𝜏ⱼᵢ⋅𝛾ᵢ,
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //   𝛾̿ⱼ ← 𝛾ⱼ₊₁,
      //   𝗳𝗼𝗿 𝑖 = 𝑗 + 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //     𝛾̿ⱼ ← 𝛾̿ⱼ + 𝜏ⱼᵢ⋅𝛾ᵢ₊₁.
      //   𝗲𝗻𝗱 𝗳𝗼𝗿
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      omega_ = gamma_[l] = gamma_bar_[l], rho_ *= -omega_;
      for (size_t j = l - 1; j != 0; --j) {
        gamma_[j] = gamma_bar_[j];
        for (size_t i = j + 1; i <= l; ++i) {
          gamma_[j] -= tau_[j, i] * gamma_[i];
        }
      }
      for (size_t j = 1; j < l; ++j) {
        gamma_bar_bar_[j] = gamma_[j + 1];
        for (size_t i = j + 1; i < l; ++i) {
          gamma_bar_bar_[j] += tau_[j, i] * gamma_[i + 1];
        }
      }

      // Update the solution and the residual again:
      // ----------------------
      // 𝒙 ← 𝒙 + 𝛾₁⋅𝒓₀,
      // 𝒓₀ ← 𝒓₀ - 𝛾̅ₗ⋅𝒓ₗ,
      // 𝒖₀ ← 𝒖₀ - 𝛾ₗ⋅𝒖ₗ,
      // 𝗳𝗼𝗿 𝑗 = 𝟣, 𝑙 - 𝟣 𝗱𝗼:
      //   𝒙 ← 𝒙 + 𝛾̿ⱼ⋅𝒓ⱼ,
      //   𝒓₀ ← 𝒓₀ - 𝛾̅ⱼ⋅𝒓ⱼ,
      //   𝒖₀ ← 𝒖₀ - 𝛾ⱼ⋅𝒖ⱼ.
      // 𝗲𝗻𝗱 𝗳𝗼𝗿
      // ----------------------
      V.add_assign(x_vec, r_vecs_[0], gamma_[1]);
      V.sub_assign(r_vecs_[0], r_vecs_[l], gamma_bar_[l]);
      V.sub_assign(u_vecs_[0], u_vecs_[l], gamma_[l]);
      for (size_t j = 1; j < l; ++j) {
        V.add_assign(x_vec, r_vecs_[j], gamma_bar_bar_[j]);
        V.sub_assign(r_vecs_[0], r_vecs_[j], gamma_bar_[j]);
        V.sub_assign(u_vecs_[0], u_vecs_[j], gamma_[j]);
      }
    }

    return V.norm(r_vecs_[0]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num              alpha_, rho_, omega_;
  mutable std::vector<Num> gamma_, gamma_bar_, gamma_bar_bar_, sigma_;
  mutable Mdvector<Num, 2> tau_;
  mutable Vec              r_tilde_vec_, z_vec_;
  mutable std::vector<Vec> r_vecs_, u_vecs_;

}; // class BiCGStabLSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
