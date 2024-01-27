/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math_utils.hpp"
#include "tit/core/meta.hpp"
#include "tit/par/thread.hpp"
#include "tit/sph/TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph::fsi {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Equation that links Euler strain tensor to the Cauchy stress tensor.
\******************************************************************************/
template<class Derived>
class EquationOfState {
public:
}; // class EquationOfState

/** Equation of state type. */
template<class DerivedEOS>
concept equation_of_state =
    std::movable<DerivedEOS> &&
    std::derived_from<DerivedEOS, EquationOfState<DerivedEOS>>;

/******************************************************************************\
 ** Hooke’s law equation of state.
\******************************************************************************/
class HookesLaw final : public EquationOfState<HookesLaw> {
private:

  real_t E_s_ = 2.0e+6, nu_s_ = 0.4;

public:

  /** Compute Cauchy stress tensor from Euler strain tensor. */
  template<class Real, size_t Dim>
  constexpr auto stress_tensor(Mat<Real, Dim> const& eps) const noexcept {
    static_assert(Dim == 2);
    Mat<Real, Dim> sigma;
    sigma[0, 0] = eps[0, 0] + nu_s_ * eps[1, 1];
    sigma[1, 1] = nu_s_ * eps[0, 0] + eps[1, 1];
    sigma[0, 1] = (1 - nu_s_) * eps[0, 1];
    sigma[1, 0] = sigma[0, 1];
    return std::tuple{E_s_ / (1.0 - pow2(nu_s_)), sigma};
  }

  template<class PV>
  constexpr auto sound_speed(PV a) const noexcept {
    auto const K_s = E_s_ / (3.0 * (1.0 - 2.0 * nu_s_));
    auto const cs_0 = sqrt(K_s / rho[a]);
    return cs_0;
  }

}; // class HookesLaw

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// We reuse the same kernels as for fluid SPH.
using tit::sph::kernel;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// We reuse the same artificial viscosity as for fluid SPH.
using tit::sph::artificial_viscosity;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<equation_of_state EquationOfState, kernel Kernel,
         artificial_viscosity ArtificialViscosity>
class StructureEquations {
private:

  EquationOfState eos_;
  Kernel kernel_;
  ArtificialViscosity artvisc_;
  mutable bool initialized_ = false;

public:

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed, parinfo} | // TODO: fixed should not be here.
      meta::Set{h, m, rho, P, cs, r, r_0, v, dv_dt, L} |
      Kernel::required_fields | ArtificialViscosity::required_fields;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Initialize structure equations. */
  constexpr explicit StructureEquations(
      EquationOfState eos = {}, Kernel kernel = {},
      ArtificialViscosity artvisc = {}) noexcept
      : eos_{std::move(eos)}, kernel_{std::move(kernel)},
        artvisc_{std::move(artvisc)} {}

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init([[maybe_unused]] ParticleArray& particles) const {}

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index(ParticleArray& particles,
                       ParticleAdjacency& adjacent_particles) const {
    // In Total Lagrangian SPH reference state is captured just once.
    if (initialized_) return;
    initialized_ = true;
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return kernel_.radius(a); });
    // Store the reference state.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Store initial particle positions.
      r_0[a] = r[a];
      /// Clean the renormalization matrix.
      L[a] = {};
    });
    // Compute kernel gradient renormalization matrix.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      auto const grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      auto const V0_a = m[a] / rho[a], V0_b = m[b] / rho[b];
      /// Update kernel gradient renormalization matrix.
      auto const L_flux = outer(r_0[b, a], grad0_W_ab);
      L[a] += V0_b * L_flux, L[b] += V0_a * L_flux;
    });
    par::static_for_each(particles.views(), [&](PV a) {
      /// Finalize kernel gradient renormalization matrix.
      auto const L_a_inv = MatInv{L[a]};
      if (!is_zero(L_a_inv.det())) L[a] = transpose(L_a_inv());
      else L[a] = 1.0;
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute density-related fields. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_density(
      [[maybe_unused]] ParticleArray& particles,
      [[maybe_unused]] ParticleAdjacency& adjacent_particles) const {}

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute velocity related fields. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Clean velocity-related fields.
      dv_dt[a] = {};
      P[a] = {};
      /// Compute sound speed.
      cs[a] = eos_.sound_speed(a);
    });
    // Compute tensor of deformation gradient and artificial viscous force.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      auto const grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      auto const V0_a = m[a] / rho[a], V0_b = m[b] / rho[b];
      /// Update tensor of deformation gradient (stored in `P`).
      auto const P_flux = outer(r[b, a], grad0_W_ab);
      P[a] += V0_b * P_flux, P[b] += V0_a * P_flux;
      /// Update artificial viscous force.
      auto const Pi_ab = artvisc_.velocity_term(a, b);
      auto const v_flux = Pi_ab * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
    });
    par::static_for_each(particles.views(), [&](PV a) {
      /// Finalize tensor of deformation gradient (stored in `P`)
      /// and compute auxiliary tensors from it.
      auto const F_a = P[a] * L[a];
      auto const F_T_a = transpose(F_a);
      auto const F_T_inv_a = MatInv{F_T_a};
      auto const F_invT_a = F_T_inv_a();
      auto const J_a = F_T_inv_a.tdet();
      /// Compute Green-Lagrange strain tensor.
      constexpr auto I = decltype(F_a)(1.0);
      auto const E_a = 0.5 * (F_T_a * F_a - I);
      /// Compute Euler strain tensor.
      auto const eps_a = F_invT_a * E_a * F_T_a;
      /// Cauchy stress tensor.
      auto [weight_a, sigma_a] = eos_.stress_tensor(eps_a);
      sigma_a *= weight_a / pow2(rho[a]);
      /// Compute Piola-Kirchhoff stress tensor.
      P[a] = J_a * sigma_a * F_invT_a;
      /// Finalize artificial viscous force.
      dv_dt[a] = J_a * F_invT_a * dv_dt[a];
    });
    // Compute velocity time derivative.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      auto const grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      /// Update velocity time derivative.
      auto const v_flux = (P[a] + P[b]) * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
    });
    par::static_for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
    });
  }

}; // class StructureEquations

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph::fsi
