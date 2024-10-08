/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph::fsi {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Equation that links Euler strain tensor to the Cauchy stress tensor.
template<class Derived>
class EquationOfState {};

/// Equation of state type.
template<class DerivedEOS>
concept equation_of_state =
    std::movable<DerivedEOS> &&
    std::derived_from<DerivedEOS, EquationOfState<DerivedEOS>>;

/// Hooke’s law equation of state.
class HookesLaw final : public EquationOfState<HookesLaw> {
public:

  /// Compute Cauchy stress tensor from Euler strain tensor.
  template<class Real, size_t Dim>
  constexpr auto stress_tensor(const Mat<Real, Dim>& eps) const noexcept {
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
    const auto K_s = E_s_ / (3.0 * (1.0 - 2.0 * nu_s_));
    const auto cs_0 = sqrt(K_s / rho[a]);
    return cs_0;
  }

private:

  real_t E_s_ = 2.0e+6;
  real_t nu_s_ = 0.4;

}; // class HookesLaw

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// We reuse the same kernels as for fluid SPH.
using tit::sph::kernel;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// We reuse the same artificial viscosity as for fluid SPH.
using tit::sph::artificial_viscosity;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The particle estimator with a fixed kernel width.
template<equation_of_state EquationOfState,
         kernel Kernel,
         artificial_viscosity ArtificialViscosity>
class StructureEquations {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      meta::Set{parinfo} | // TODO: parinfo should not be here.
      meta::Set{h, m, rho, P, cs, r, r_0, v, dv_dt, L} |
      Kernel::required_fields | ArtificialViscosity::required_fields;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize structure equations.
  constexpr explicit StructureEquations(
      EquationOfState eos = {},
      Kernel kernel = {},
      ArtificialViscosity artvisc = {}) noexcept
      : eos_{std::move(eos)}, kernel_{std::move(kernel)},
        artvisc_{std::move(artvisc)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_array<required_fields> ParticleArray>
  constexpr void init(ParticleArray& /*particles*/) const {
    // Nothing to do here.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr auto index(ParticleMesh& mesh, ParticleArray& particles) const {
    // In Total Lagrangian SPH reference state is captured just once.
    if (initialized_) return;
    initialized_ = true;
    using PV = ParticleView<ParticleArray>;
    mesh.update(particles, [&](PV a) { return kernel_.radius(a); });
    // Store the reference state.
    par::for_each(particles.all(), [](PV a) {
      /// Store initial particle positions.
      r_0[a] = r[a];
      /// Clean the renormalization matrix.
      L[a] = {};
    });
    // Compute kernel gradient renormalization matrix.
    par::block_for_each(mesh.block_pairs(particles), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      const auto V0_a = m[a] / rho[a];
      const auto V0_b = m[b] / rho[b];
      /// Update kernel gradient renormalization matrix.
      const auto L_flux = outer(r_0[b, a], grad0_W_ab);
      L[a] += V0_b * L_flux, L[b] += V0_a * L_flux;
    });
    par::for_each(particles.all(), [](PV a) {
      /// Finalize kernel gradient renormalization matrix.
      const auto fact = lu(L[a]);
      if (fact) L[a] = transpose(fact->inverse());
      else L[a] = eye(L[a]);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& /*particles*/
  ) const {
    // Nothing to do here.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_forces(ParticleMesh& mesh,
                                ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::for_each(particles.all(), [this](PV a) {
      /// Clean velocity-related fields.
      dv_dt[a] = {};
      P[a] = {};
      /// Compute sound speed.
      cs[a] = eos_.sound_speed(a);
    });
    // Compute tensor of deformation gradient and artificial viscous force.
    par::block_for_each(mesh.block_pairs(particles), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      const auto V0_a = m[a] / rho[a];
      const auto V0_b = m[b] / rho[b];
      /// Update tensor of deformation gradient (stored in `P`).
      const auto P_flux = outer(r[b, a], grad0_W_ab);
      P[a] += V0_b * P_flux, P[b] += V0_a * P_flux;
      /// Update artificial viscous force.
      const auto Pi_ab = artvisc_.velocity_term(a, b);
      const auto v_flux = Pi_ab * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
    });
    par::for_each(particles.all(), [this](PV a) {
      /// Finalize tensor of deformation gradient (stored in `P`)
      /// and compute auxiliary tensors from it.
      const auto F_a = P[a] * L[a];
      const auto F_T_a = transpose(F_a);
      const auto F_T_fact_a = lu(F_T_a);
      const auto F_invT_a = F_T_fact_a->inverse();
      const auto J_a = F_T_fact_a->det();
      /// Compute Green-Lagrange strain tensor.
      const auto I = eye(F_a);
      const auto E_a = 0.5 * (F_T_a * F_a - I);
      /// Compute Euler strain tensor.
      const auto eps_a = F_invT_a * E_a * F_T_a;
      /// Cauchy stress tensor.
      auto [weight_a, sigma_a] = eos_.stress_tensor(eps_a);
      sigma_a *= weight_a / pow2(rho[a]);
      /// Compute Piola-Kirchhoff stress tensor.
      P[a] = J_a * sigma_a * F_invT_a;
      /// Finalize artificial viscous force.
      dv_dt[a] = J_a * F_invT_a * dv_dt[a];
    });
    // Compute velocity time derivative.
    par::block_for_each(mesh.block_pairs(particles), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      /// Update velocity time derivative.
      const auto v_flux = (P[a] + P[b]) * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
    });
    par::for_each(particles.fluid(), []([[maybe_unused]] PV a) {
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;
  [[no_unique_address]] ArtificialViscosity artvisc_;
  mutable bool initialized_ = false;

}; // class StructureEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph::fsi
