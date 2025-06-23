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

/// The @c IDR(s) (Induced Dimension Reduction) linear operator equation solver.
///
/// References:
/// @verbatim
/// [1] Peter Sonneveld, Martin B. van Gijzen.
///     “IDR(s): A Family of Simple and Fast Algorithms for Solving
///      Large Nonsymmetric Systems of Linear Equations.”
///     SIAM J. Sci. Comput. 31 (2008): 1035-1062.
/// [2] Martin B. van Gijzen, Peter Sonneveld.
///     “Algorithm 913: An Elegant IDR(s) Variant that Efficiently
///      Exploits Biorthogonality Properties.”
///     ACM Trans. Math. Softw. 38 (2011): 5:1-5:19.
/// @endverbatim
template<class Mapping, class Preconditioner>
class IDRSSolver final :
    public InnerOuterIterativeSolver<Mapping, Preconditioner> {
public:

  using Base = InnerOuterIterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a @c IDR(s) solver.
  constexpr IDRSSolver(const Mapping& A, const Preconditioner& P) : Base{A, P} {
    // Let's tweak some defaults.
    this->set_num_inner_iterations(4);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  s = this->num_inner_iterations();

    phi_.resize(s);
    gamma_.resize(s);
    mu_.assign(s, s);

    V.init(r_vec_, x_vec);
    V.init(v_vec_, x_vec);
    if (this->has_preconditioner()) V.init(z_vec_, x_vec);

    p_vecs_.resize(s);
    for (auto& p_vec : p_vecs_) V.init(p_vec, x_vec);
    u_vecs_.resize(s);
    for (auto& u_vec : u_vecs_) V.init(u_vec, x_vec);
    g_vecs_.resize(s);
    for (auto& g_vec : g_vecs_) V.init(g_vec, x_vec);

    // Initialize:
    // ----------------------
    // 𝒓 ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒓,
    //   𝒓 ← 𝓟𝒛.
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝜑₀ ← ‖𝒓‖.
    // ----------------------
    A.residual(r_vec_, b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(z_vec_, r_vec_);
      P.apply(r_vec_, z_vec_);
    }
    phi_[0] = V.norm(r_vec_);

    return phi_[0];
  }

  constexpr void inner_init_(Vec& /*x_vec*/, const Vec& /*b_vec*/) const {
    const auto& V = this->space();
    const auto  s = this->num_inner_iterations();

    // Build shadow space and initialize 𝜑:
    // ----------------------
    // 𝗶𝗳 𝘍𝘪𝘳𝘴𝘵𝘐𝘵𝘦𝘳𝘢𝘵𝘪𝘰𝘯:
    //   𝜔 ← 𝜇₀₀ ← 𝟣,
    //   𝒑₀ ← 𝒓/𝜑₀,
    //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑠 𝗱𝗼:
    //     𝜇ᵢᵢ ← 𝟣, 𝜑ᵢ ← 𝟢,
    //     𝒑ᵢ ← 𝘙𝘢𝘯𝘥𝘰𝘮,
    //     𝗳𝗼𝗿 𝑗 = 𝟢, 𝑖 - 𝟣 𝗱𝗼:
    //       𝜇ᵢⱼ ← 𝟢,
    //       𝒑ᵢ ← 𝒑ᵢ - <𝒑ᵢ⋅𝒑ⱼ>⋅𝒑ⱼ,
    //     𝗲𝗻𝗱 𝗳𝗼𝗿
    //     𝒑ᵢ ← 𝒑ᵢ/‖𝒑ᵢ‖.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑠 - 𝟣 𝗱𝗼:
    //     𝜑ᵢ ← <𝒑ᵢ⋅𝒓>.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (this->iteration() == 0) {
      omega_ = mu_[0, 0] = Num{1};
      V.scale(p_vecs_[0], r_vec_, Num{1} / phi_[0]);
      for (size_t i = 1; i < s; ++i) {
        mu_[i, i] = Num{1}, phi_[i] = Num{0};
        V.rand_fill(p_vecs_[i]);
        for (size_t j = 0; j < i; ++j) {
          mu_[i, j] = Num{0};
          V.sub_assign(p_vecs_[i], p_vecs_[j], V.dot(p_vecs_[i], p_vecs_[j]));
        }
        V.scale_assign(p_vecs_[i], Num{1} / V.norm(p_vecs_[i]));
      }
    } else {
      for (size_t i = 0; i < s; ++i) {
        phi_[i] = V.dot(p_vecs_[i], r_vec_);
      }
    }
  }

  constexpr auto inner_iterate_(Vec& x_vec, const Vec& /*b_vec*/) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  s = this->num_inner_iterations();
    const auto  k = this->inner_iteration();

    // Compute 𝛾:
    // ----------------------
    // 𝛾ₖ:ₛ₋₁ ← (𝜇ₖ:ₛ₋₁,ₖ:ₛ₋₁)⁻¹⋅𝜑ₖ:ₛ₋₁.
    // ----------------------
    for (size_t i = k; i < s; ++i) {
      gamma_[i] = phi_[i];
      for (size_t j = k; j < i; ++j) {
        gamma_[i] -= mu_[i, j] * gamma_[j];
      }
      gamma_[i] /= mu_[i, i];
    }

    // Compute the new 𝒈ₖ and 𝒖ₖ vectors:
    // ----------------------
    // 𝒗 ← 𝒓 - 𝛾ₖ⋅𝒈ₖ,
    // 𝗳𝗼𝗿 𝑖 = 𝑘 + 𝟣, 𝑠 - 𝟣 𝗱𝗼:
    //   𝒗 ← 𝒗 - 𝛾ᵢ⋅𝒈ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝒛 ← 𝒗,
    //   𝒗 ← 𝓟𝒛,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝒖ₖ ← 𝜔⋅𝒗 + 𝛾ₖ⋅𝒖ₖ,
    // 𝗳𝗼𝗿 𝑖 = 𝑘 + 𝟣, 𝑠 - 𝟣 𝗱𝗼:
    //   𝒖ₖ ← 𝒖ₖ + 𝛾ᵢ⋅𝒖ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒈ₖ ← 𝓟(𝒛 ← 𝓐𝒖ₖ).
    // 𝗲𝗹𝘀𝗲:
    //   𝒈ₖ ← 𝓐𝒖ₖ.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    V.sub(v_vec_, r_vec_, g_vecs_[k], gamma_[k]);
    for (size_t i = k + 1; i < s; ++i) {
      V.sub_assign(v_vec_, g_vecs_[i], gamma_[i]);
    }
    if (this->has_right_preconditioner()) {
      V.swap(z_vec_, v_vec_);
      P.apply(v_vec_, z_vec_);
    }
    V.add(u_vecs_[k], u_vecs_[k], gamma_[k], v_vec_, omega_);
    for (size_t i = k + 1; i < s; ++i) {
      V.add_assign(u_vecs_[k], u_vecs_[i], gamma_[i]);
    }
    if (this->has_left_preconditioner()) {
      P.chapply(g_vecs_[k], z_vec_, A, u_vecs_[k]);
    } else {
      A.apply(g_vecs_[k], u_vecs_[k]);
    }

    // Biorthogonalize the new vectors 𝒈ₖ and 𝒖ₖ:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 - 𝟣 𝗱𝗼:
    //   𝛼 ← <𝒑ᵢ⋅𝒈ₖ>/𝜇ᵢᵢ,
    //   𝒖ₖ ← 𝒖ₖ - 𝛼⋅𝒖ᵢ,
    //   𝒈ₖ ← 𝒈ₖ - 𝛼⋅𝒈ᵢ.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    for (size_t i = 0; i < k; ++i) {
      const auto alpha = safe_divide(V.dot(p_vecs_[i], g_vecs_[k]), mu_[i, i]);
      V.sub_assign(u_vecs_[k], u_vecs_[i], alpha);
      V.sub_assign(g_vecs_[k], g_vecs_[i], alpha);
    }

    // Compute the new column of 𝜇:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝑘, 𝑠 - 𝟣 𝗱𝗼:
    //   𝜇ᵢₖ ← <𝒑ᵢ⋅𝒈ₖ>.
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // ----------------------
    for (size_t i = k; i < s; ++i) {
      mu_[i, k] = V.dot(p_vecs_[i], g_vecs_[k]);
    }

    // Update the solution and the residual:
    // ----------------------
    // 𝛽 ← 𝜑ₖ/𝜇ₖₖ,
    // 𝒙 ← 𝒙 + 𝛽⋅𝒖ₖ,
    // 𝒓 ← 𝒓 - 𝛽⋅𝒈ₖ.
    // ----------------------
    const auto beta = safe_divide(phi_[k], mu_[k, k]);
    V.add_assign(x_vec, u_vecs_[k], beta);
    V.sub_assign(r_vec_, g_vecs_[k], beta);

    // Update 𝜑:
    // ----------------------
    // 𝜑ₖ₊₁:ₛ₋₁ ← 𝜑ₖ₊₁:ₛ₋₁ - 𝛽⋅𝜇ₖ₊₁:ₛ₋₁,ₖ.
    // ----------------------
    for (size_t i = k + 1; i < s; ++i) {
      phi_[i] -= beta * mu_[i, k];
    }

    if (k == s - 1) {
      // Enter the next 𝓖 subspace:
      // ----------------------
      // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
      //   𝒗 ← 𝓟(𝒛 ← 𝓐𝒓),
      // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
      //   𝒗 ← 𝓐(𝒛 ← 𝓟𝒓),
      // 𝗲𝗹𝘀𝗲:
      //   𝒗 ← 𝓐𝒓,
      // 𝗲𝗻𝗱 𝗶𝗳
      // 𝜔 ← <𝒗⋅𝒓>/<𝒗⋅𝒗>,
      // 𝒙 ← 𝒙 + 𝜔⋅(𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦 ? 𝒛 : 𝒓),
      // 𝒓 ← 𝒓 - 𝜔⋅𝒗.
      // ----------------------
      if (this->has_left_preconditioner()) {
        P.chapply(v_vec_, z_vec_, A, r_vec_);
      } else if (this->has_right_preconditioner()) {
        A.chapply(v_vec_, z_vec_, P, r_vec_);
      } else {
        A.apply(v_vec_, r_vec_);
      }
      omega_ = safe_divide(V.dot(v_vec_, r_vec_), V.dot(v_vec_, v_vec_));
      V.add_assign(x_vec,
                   this->has_right_preconditioner() ? z_vec_ : r_vec_,
                   omega_);
      V.sub_assign(r_vec_, v_vec_, omega_);
    }

    return V.norm(r_vec_);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable Num              omega_;
  mutable std::vector<Num> phi_, gamma_;
  mutable Mdvector<Num, 2> mu_;
  mutable Vec              r_vec_, v_vec_, z_vec_;
  mutable std::vector<Vec> p_vecs_, u_vecs_, g_vecs_;

}; // class IDRSSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
