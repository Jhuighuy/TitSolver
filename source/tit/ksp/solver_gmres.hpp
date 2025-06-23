/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/core/math.hpp"

#include "tit/ksp/solver.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base class for @c GMRES and @c FGMRES.
template<bool Flexible, class Mapping, class Preconditioner>
class BaseGMRESSolver :
    public InnerOuterIterativeSolver<Mapping, Preconditioner> {
public:

  using Base = InnerOuterIterativeSolver<Mapping, Preconditioner>;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto outer_init_(Vec& x_vec, const Vec& b_vec) const -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  m = this->num_inner_iterations();

    beta_.resize(m + 1);
    cs_.resize(m);
    sn_.resize(m);
    H_.assign(m + 1, m);

    q_vecs_.resize(m + 1);
    for (auto& q_vec : q_vecs_) V.init(q_vec, x_vec);

    if (this->has_preconditioner()) {
      z_vecs_.resize(Flexible ? m : 1);
      for (auto& z_vec : z_vecs_) V.init(z_vec, x_vec);
    }

    // Initialize:
    // ----------------------
    // 𝒒₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛₀ ← 𝒒₀,
    //   𝒒₀ ← 𝓟𝒛₀,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛽₀ ← ‖𝒒₀‖,
    // 𝒒₀ ← 𝒒₀/𝛽₀.
    // ----------------------
    A.residual(q_vecs_[0], b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(z_vecs_[0], q_vecs_[0]);
      P.apply(q_vecs_[0], z_vecs_[0]);
    }
    beta_[0] = V.norm(q_vecs_[0]);
    V.scale_assign(q_vecs_[0], Num{1} / beta_[0]);

    return beta_[0];
  }

  constexpr void inner_init_(Vec& x_vec, const Vec& b_vec) const {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();

    // Initialize:
    // ----------------------
    // 𝒒₀ ← 𝒃 - 𝓐𝒙,
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒛₀ ← 𝒒₀,
    //   𝒒₀ ← 𝓟𝒛₀,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝛽₀ ← ‖𝒒₀‖,
    // 𝒒₀ ← 𝒒₀/𝛽₀.
    // ----------------------
    A.residual(q_vecs_[0], b_vec, x_vec);
    if (this->has_left_preconditioner()) {
      V.swap(z_vecs_[0], q_vecs_[0]);
      P.apply(q_vecs_[0], z_vecs_[0]);
    }
    beta_[0] = V.norm(q_vecs_[0]);
    V.scale_assign(q_vecs_[0], Num{1} / beta_[0]);
  }

  constexpr auto inner_iterate_(Vec& /*x_vec*/, const Vec& /*b_vec*/) const
      -> Num {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto& P = this->preconditioner();
    const auto  k = this->inner_iteration();

    // Compute the new 𝒒ₖ₊₁ vector:
    // ----------------------
    // 𝗶𝗳 𝘓𝘦𝘧𝘵𝘗𝘳𝘦:
    //   𝒒ₖ₊₁ ← 𝓟(𝒛₀ ← 𝓐𝒒ₖ),
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝑗 ← 𝘍𝘭𝘦𝘹𝘪𝘣𝘭𝘦 ? 𝑘 : 𝟢,
    //   𝒒ₖ₊₁ ← 𝓐(𝒛ⱼ ← 𝓟𝒒ₖ),
    // 𝗲𝗹𝘀𝗲:
    //   𝒒ₖ₊₁ ← 𝓐𝒒ₖ,
    // 𝗲𝗻𝗱 𝗶𝗳
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //   𝐻ᵢₖ ← <𝒒ₖ₊₁⋅𝒒ᵢ>,
    //   𝒒ₖ₊₁ ← 𝒒ₖ₊₁ - 𝐻ᵢₖ⋅𝒒ᵢ,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝐻ₖ₊₁,ₖ ← ‖𝒒ₖ₊₁‖,
    // 𝒒ₖ₊₁ ← 𝒒ₖ₊₁/𝐻ₖ₊₁,ₖ.
    // ----------------------
    if (this->has_left_preconditioner()) {
      P.chapply(q_vecs_[k + 1], z_vecs_[0], A, q_vecs_[k]);
    } else if (this->has_right_preconditioner()) {
      const auto j = Flexible ? k : 0UZ;
      A.chapply(q_vecs_[k + 1], z_vecs_[j], P, q_vecs_[k]);
    } else {
      A.apply(q_vecs_[k + 1], q_vecs_[k]);
    }
    for (size_t i = 0; i <= k; ++i) {
      H_[i, k] = V.dot(q_vecs_[k + 1], q_vecs_[i]);
      V.sub_assign(q_vecs_[k + 1], q_vecs_[i], H_[i, k]);
    }
    H_[k + 1, k] = V.norm(q_vecs_[k + 1]);
    V.scale_assign(q_vecs_[k + 1], Num{1} / H_[k + 1, k]);

    // Eliminate the last element in 𝐻 and and update the rotation matrix:
    // ----------------------
    // 𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 - 𝟣 𝗱𝗼:
    //   𝜒 ← 𝑐𝑠ᵢ⋅𝐻ᵢₖ + 𝑠𝑛ᵢ⋅𝐻ᵢ₊₁,ₖ,
    //   𝐻ᵢ₊₁,ₖ ← -𝑠𝑛ᵢ⋅𝐻ᵢₖ + 𝑐𝑠ᵢ⋅𝐻ᵢ₊₁,ₖ,
    //   𝐻ᵢₖ ← 𝜒,
    // 𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝑐𝑠ₖ, 𝑠𝑛ₖ ← 𝘚𝘺𝘮𝘖𝘳𝘵𝘩𝘰(𝐻ₖₖ, 𝐻ₖ₊₁,ₖ),
    // 𝐻ₖₖ ← 𝑐𝑠ₖ⋅𝐻ₖₖ + 𝑠𝑛ₖ⋅𝐻ₖ₊₁,ₖ,
    // 𝐻ₖ₊₁,ₖ ← 𝟢.
    // ----------------------
    for (size_t i = 0; i < k; ++i) {
      const auto chi = +cs_[i] * H_[i, k] + sn_[i] * H_[i + 1, k];
      H_[i + 1, k]   = -sn_[i] * H_[i, k] + cs_[i] * H_[i + 1, k];
      H_[i, k]       = chi;
    }
    std::tie(cs_[k], sn_[k], std::ignore) = sym_ortho(H_[k, k], H_[k + 1, k]);
    H_[k, k]     = cs_[k] * H_[k, k] + sn_[k] * H_[k + 1, k];
    H_[k + 1, k] = Num{0};

    // Update the 𝛽-solution and the residual norm:
    // ----------------------
    // 𝛽ₖ₊₁ ← -𝑠𝑛ₖ⋅𝛽ₖ, 𝛽ₖ ← 𝑐𝑠ₖ⋅𝛽ₖ.
    // ----------------------
    beta_[k + 1] = -sn_[k] * beta_[k], beta_[k] *= cs_[k];

    return abs(beta_[k + 1]);
  }

  constexpr void inner_finalize_(Vec& x_vec, const Vec& /*b_vec*/) const {
    const auto& V = this->space();
    const auto& A = this->mapping();
    const auto  k = this->inner_iteration();

    // Finalize the 𝛽-solution:
    // ----------------------
    // 𝛽₀:ₖ ← (𝐻₀:ₖ,₀:ₖ)⁻¹𝛽₀:ₖ.
    // ----------------------
    for (ssize_t i = k; i >= 0; --i) {
      for (size_t j = i + 1; j <= k; ++j) {
        beta_[i] -= H_[i, j] * beta_[j];
      }
      beta_[i] /= H_[i, i];
    }

    // Compute the 𝒙-solution:
    // ----------------------
    // 𝗶𝗳 𝗻𝗼𝘁 𝘙𝘪𝘨𝘩𝘵𝘗𝘳𝘦:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //     𝒙 ← 𝒙 + 𝛽ᵢ⋅𝒒ᵢ.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲 𝗶𝗳 𝘍𝘭𝘦𝘹𝘪𝘣𝘭𝘦:
    //   𝗳𝗼𝗿 𝑖 = 𝟢, 𝑘 𝗱𝗼:
    //     𝒙 ← 𝒙 + 𝛽ᵢ⋅𝒛ᵢ.
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    // 𝗲𝗹𝘀𝗲:
    //   𝒒₀ ← 𝛽₀⋅𝒒₀,
    //   𝗳𝗼𝗿 𝑖 = 𝟣, 𝑘 𝗱𝗼:
    //     𝒒₀ ← 𝒒₀ + 𝛽ᵢ⋅𝒒ᵢ,
    //   𝗲𝗻𝗱 𝗳𝗼𝗿
    //   𝒛₀ ← 𝓟𝒒₀,
    //   𝒙 ← 𝒙 + 𝒛₀.
    // 𝗲𝗻𝗱 𝗶𝗳
    // ----------------------
    if (!this->has_right_preconditioner()) {
      for (size_t i = 0; i <= k; ++i) {
        V.add_assign(x_vec, q_vecs_[i], beta_[i]);
      }
    } else if constexpr (Flexible) {
      for (size_t i = 0; i <= k; ++i) {
        V.add_assign(x_vec, z_vecs_[i], beta_[i]);
      }
    } else {
      V.scale_assign(q_vecs_[0], beta_[0]);
      for (size_t i = 1; i <= k; ++i) {
        V.add_assign(q_vecs_[0], q_vecs_[i], beta_[i]);
      }
      A.apply(z_vecs_[0], q_vecs_[0]);
      V.add_assign(x_vec, z_vecs_[0]);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

protected:

  /// Construct a @c (F)GMRES solver.
  constexpr BaseGMRESSolver(const Mapping& A, const Preconditioner& P)
      : Base{A, P} {
    if constexpr (Flexible) this->use_left_preconditioner();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  mutable std::vector<Num> beta_, cs_, sn_;
  mutable Mdvector<Num, 2> H_;
  mutable std::vector<Vec> q_vecs_;
  mutable std::vector<Vec> z_vecs_;

}; // class BaseGMRESSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The @c GMRES (Generalized Minimal Residual) linear operator equation solver.
///
/// @c GMRES is typically more robust than the @c BiCG type solvers, but it may
/// be slower than the @c BiCG solvers for the well-conditioned moderate sized
/// problems.
///
/// @c GMRES is algebraically equivalent to @c MINRES method in the self-adjoint
/// operator unpreconditioned case, however, the need for restarts may lead to
/// the much slower @c GMRES convergence rate.
///
/// @c GMRES may be applied to the singular problems, and the square least
/// squares problems, although, similarly to @c MINRES, convergeance to minimum
/// norm solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef and Martin H. Schultz.
///     “GMRES: A generalized minimal residual algorithm for solving
///      nonsymmetric linear systems.”
///     SIAM J. Sci. Stat. Comput., 7:856–869, 1986.
/// @endverbatim
template<class Mapping, class Preconditioner>
class GMRESSolver final :
    public BaseGMRESSolver</*Flexible=*/false, Mapping, Preconditioner> {
public:

  /// Construct a @c GMRES solver.
  constexpr GMRESSolver(const Mapping& A, const Preconditioner& P)
      : BaseGMRESSolver</*Flexible=*/false, Mapping, Preconditioner>{A, P} {}

}; // class GMRESSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief The @c FGMRES (Flexible Generalized Minimal Residual)
///   linear operator equation solver.
///
/// @c FGMRES is typically more robust than the @c BiCG type solvers,
///   but it may be slower than the @c BiCG solvers for the
///   well-conditioned moderate sized problems.
///
/// @c FGMRES does the same amount of operations per iteration
///   as @c GMRES, but also allows usage of the variable (or flexible)
///   preconditioners with the price of doubleing of the memory
///   usage. For the static preconditioners, @c FGMRES requires
///   one preconditioner-vector product less than @c GMRES.
///   @c FGMRES supports only the right preconditioning.
///
/// @c FGMRES may be applied to the singular problems, and the square
///   least squares problems, although, similarly to @c MINRES,
///   convergeance to minimum norm solution is not guaranteed.
///
/// References:
/// @verbatim
/// [1] Saad, Yousef.
///     “A Flexible Inner-Outer Preconditioned GMRES Algorithm.”
///     SIAM J. Sci. Comput. 14 (1993): 461-469.
/// @endverbatim
template<class Mapping, class Preconditioner>
class FGMRESSolver final :
    public BaseGMRESSolver</*Flexible=*/true, Mapping, Preconditioner> {
public:

  /// Construct a @c FGMRES solver.
  constexpr FGMRESSolver(const Mapping& A, const Preconditioner& P)
      : BaseGMRESSolver</*Flexible=*/true, Mapping, Preconditioner>{A, P} {}

}; // class FGMRESSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
