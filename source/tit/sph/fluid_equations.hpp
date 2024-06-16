/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_motion.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/viscosity.hpp"

/// @todo What do we need to clean-up?
/// [x] `ParticleArray` must use NTTPs for the fields.
/// [ ] The whole field header must be cleaned up.
/// [ ] Boundary conditions must be implemented in a separate header.
/// [x] Viscosity and heat conductivity must support non-constant
///     coefficients.
/// [ ] `ParticleMesh` must be cleaned up.
/// [ ] Clean up the Godunov implementation.
/// [ ] Clean up the FSI implementation.
/// [ ] Clean up the Runge-Kutta implementation.
/// [ ] `TitParticle` header must become an umbrella header for all the
///     particle-related headers.
/// [ ] CFL condition must be implemented.

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<equation_of_motion EquationOfMotion,
         continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         energy_equation EnergyEquation,
         equation_of_state EquationOfState,
         viscosity Viscosity,
         artificial_viscosity ArtificialViscosity,
         kernel Kernel>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      EquationOfMotion::required_fields |    //
      ContinuityEquation::required_fields |  //
      MomentumEquation::required_fields |    //
      EnergyEquation::required_fields |      //
      EquationOfState::required_fields |     //
      Viscosity::required_fields |           //
      ArtificialViscosity::required_fields | //
      Kernel::required_fields |              //
      meta::Set{fixed, parinfo} |            // TODO: fixed should not be here.
      meta::Set{h, m, rho, drho_dt, p, r, v, dv_dt, u, du_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      EquationOfMotion::modified_fields |    //
      ContinuityEquation::modified_fields |  //
      MomentumEquation::modified_fields |    //
      EnergyEquation::modified_fields |      //
      EquationOfState::modified_fields |     //
      Viscosity::modified_fields |           //
      ArtificialViscosity::modified_fields | //
      Kernel::modified_fields |              //
      meta::Set{p, rho, drho_dt, grad_rho, v, dv_dt, div_v, curl_v, u, du_dt} |
      meta::Set{S, L};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param eom Equation of motion.
  /// @param continuity_equation Continuity equation.
  /// @param momentum_equation Momentum equation.
  /// @param energy_equation Energy equation.
  /// @param equation_of_state Equation of state.
  /// @param visc Viscosity.
  /// @param artvisc Artificial viscosity.
  /// @param kernel Kernel.
  constexpr explicit FluidEquations(EquationOfMotion eom,
                                    ContinuityEquation continuity_equation,
                                    MomentumEquation momentum_equation,
                                    EnergyEquation energy_equation,
                                    EquationOfState eos,
                                    Viscosity visc,
                                    ArtificialViscosity artvisc,
                                    Kernel kernel) noexcept
      : eom_{std::move(eom)},
        continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        energy_equation_{std::move(energy_equation)}, //
        eos_{std::move(eos)},                         //
        visc_{std::move(visc)},                       //
        artvisc_{std::move(artvisc)},                 //
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_array<required_fields> ParticleArray>
  constexpr void init(ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::init()");
    using PV = ParticleView<ParticleArray>;
    par::for_each(particles.all(), [](PV a) {
      // Initialize particle artificial viscosity switch value.
      if constexpr (has<PV>(alpha)) alpha[a] = 1.0;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr auto index(ParticleMesh& mesh,
                       [[maybe_unused]] ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.build(particles, [&](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void setup_boundary(
      [[maybe_unused]] ParticleMesh& mesh,
      [[maybe_unused]] ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::setup_boundary()");
#if WITH_WALLS
    using PV = ParticleView<ParticleArray>;
    par::for_each(mesh._fixed(particles), [&](auto ia) {
      auto [i, a] = ia;
      const auto search_point = a[r];
      const auto clipped_point = Domain.clamp(search_point);
      const auto r_a = 2 * clipped_point - search_point;
      real_t S = {};
      Mat<real_t, 3> M{};
      constexpr auto SCALE = 3;
      std::ranges::for_each(mesh[nullptr, particles, i], [&](PV b) {
        const auto r_ab = r_a - r[b];
        const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
        const auto W_ab = kernel_(r_ab, SCALE * h[a]);
        S += W_ab * m[b] / rho[b];
        M += outer(B_ab, B_ab * W_ab * m[b] / rho[b]);
      });
      const auto fact = ldl(M);
      if (fact) {
        Vec<real_t, 3> e{1.0, 0.0, 0.0};
        auto E = fact->solve(e);
        rho[a] = {};
        v[a] = {};
        u[a] = {};
        std::ranges::for_each(mesh[nullptr, particles, i], [&](PV b) {
          const auto r_ab = r_a - r[b];
          const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
          auto W_ab = dot(E, B_ab) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
          u[a] += m[b] / rho[b] * u[b] * W_ab;
        });
      } else if (!is_tiny(S)) {
        rho[a] = {};
        v[a] = {};
        u[a] = {};
        std::ranges::for_each(mesh[nullptr, particles, i], [&](PV b) {
          const auto r_ab = r_a - r[b];
          auto W_ab = (1 / S) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
          u[a] += m[b] / rho[b] * u[b] * W_ab;
        });
      } else {
        goto fail;
      }
      {
        const auto N = normalize(search_point - clipped_point);
        const auto D = norm(r_a - r[a]);
        // drho/dn = rho_0/(cs_0^2)*dot(g,n).
#if EASY_DAM_BREAKING
        constexpr auto rho_0 = 1000.0;
        constexpr auto cs_0 = 20 * sqrt(9.81 * 0.6);
#elif HARD_DAM_BREAKING
        constexpr auto rho_0 = 1000.0;
        constexpr auto cs_0 = 120.0;
#endif
#if WITH_GRAVITY
        constexpr auto G = Vec{0.0, -9.81};
        rho[a] += D * rho_0 / pow2(cs_0) * dot(G, N);
#endif
#if EASY_DAM_BREAKING
        { // SLIP WALL.
          auto Vn = dot(v[a], N) * N;
          auto Vt = v[a] - Vn;
          v[a] = Vt - Vn;
        }
#elif HARD_DAM_BREAKING
        { // NOSLIP WALL.
          v[a] *= -1;
        }
#endif
      }
    fail:
    });
#endif
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_density(ParticleMesh& mesh,
                                 ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    TIT_PROFILE_SECTION("FluidEquations::compute_density()");

    // Clean-up density fields.
    par::for_each(particles.all(), [](PV a) {
      /// Density fields.
      drho_dt[a] = {};
      if constexpr (has<PV>(grad_rho)) grad_rho[a] = {};
      // Renormalization fields.
      if constexpr (has<PV>(S)) S[a] = {};
      if constexpr (has<PV>(L)) L[a] = {};
    });

    // Compute density gradient and renormalization fields, if necessary.
    if constexpr (has<PV>(grad_rho) || has<PV>(S) || has<PV>(L)) {
      par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
        const auto [a, b] = ab;
        const auto V_a = m[a] / rho[a];
        const auto V_b = m[b] / rho[b];
        [[maybe_unused]] const auto W_ab = kernel_(a, b);
        [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b);

        // Update density gradient, if necessary.
        if constexpr (has<PV>(grad_rho)) {
          const auto grad_flux = rho[b, a] * grad_W_ab;
          grad_rho[a] += V_b * grad_flux;
          grad_rho[b] += V_a * grad_flux;
        }
        // Update kernel renormalization coefficient, if
        // necessary.
        if constexpr (has<PV>(S)) {
          const auto S_flux = W_ab;
          S[a] += V_b * S_flux;
          S[b] += V_a * S_flux;
        }
        // Update kernel gradient renormalization matrix, if
        // necessary.
        if constexpr (has<PV>(L)) {
          const auto L_flux = outer(r[b, a], grad_W_ab);
          L[a] += V_b * L_flux;
          L[b] += V_a * L_flux;
        }
      });
    }

    // Renormalize fields, if necessary.
    if constexpr (has<PV>(S) || has<PV>(L)) {
      par::for_each(particles.all(), [](PV a) {
        // Do not renormalize fixed particles.
        if (fixed[a]) return;
        // Renormalize density, if possible.
        if constexpr (has<PV>(S)) {
          if (!is_tiny(S[a])) rho[a] /= S[a];
        }
        // Renormalize density gradient, if possible.
        if constexpr (has<PV>(L)) {
          const auto fact = ldl(L[a]);
          if (fact) grad_rho[a] = fact->solve(grad_rho[a]);
        }
      });
    }

    // Compute density time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);
      /// Update density time derivative.
      const auto Psi_ab = artvisc_.density_term(a, b);
      drho_dt[a] -= m[b] * dot(v[b, a] - Psi_ab / rho[b], grad_W_ab);
      drho_dt[b] -= m[a] * dot(v[b, a] + Psi_ab / rho[a], grad_W_ab);
    });

    // Compute continuity equation source terms.
    par::for_each(particles.all(), [this](PV a) {
      // No source terms for fixed particles.
      if (fixed[a]) return;
      // Apply continuity equation source terms.
      std::apply([a](const auto&... f) { ((drho_dt[a] += f(a)), ...); },
                 continuity_equation_.mass_sources());
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_forces(ParticleMesh& mesh,
                                ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    TIT_PROFILE_SECTION("FluidEquations::compute_forces()");

    // Prepare velocity-related fields.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up velocity fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
      // Clean-up energy fields.
      if constexpr (has<PV>(du_dt)) du_dt[a] = {};
      // Compute pressure and sound speed, if necessary.
      p[a] = eos_.pressure(a);
      if constexpr (has<PV>(cs)) cs[a] = eos_.sound_speed(a);
    });

    // Compute velocity divergence and curl, if necessary.
    // Those fields may be required by the artificial viscosity.
    if constexpr (has<PV>(div_v) || has<PV>(curl_v)) {
      par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
        const auto [a, b] = ab;
        const auto V_a = m[a] / rho[a];
        const auto V_b = m[b] / rho[b];
        const auto grad_W_ab = kernel_.grad(a, b);
        // Update velocity divergence, if necessary.
        if constexpr (has<PV>(div_v)) {
          const auto div_flux = dot(v[b, a], grad_W_ab);
          div_v[a] += V_b * div_flux;
          div_v[b] += V_a * div_flux;
        }
        // Update velocity curl, if necessary.
        if constexpr (has<PV>(curl_v)) {
          const auto curl_flux = -cross(v[b, a], grad_W_ab);
          curl_v[a] += V_b * curl_flux;
          curl_v[b] += V_a * curl_flux;
        }
      });
    }

    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);
      // Update velocity time derivative.
      const auto P_a = p[a] / pow2(rho[a]);
      const auto P_b = p[b] / pow2(rho[b]);
      const auto Pi_ab = visc_(a, b) + artvisc_.velocity_term(a, b);
      const auto v_flux = (-P_a - P_b + Pi_ab) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;
      // Update internal energy time derivative, if necessary.
      if constexpr (has<PV>(du_dt)) {
        const auto Q_ab = energy_equation_.heat_conductivity()(a, b);
        du_dt[a] -= m[b] * dot((P_a - Pi_ab / 2) * v[b, a] - Q_ab, grad_W_ab);
        du_dt[b] -= m[a] * dot((P_b - Pi_ab / 2) * v[b, a] + Q_ab, grad_W_ab);
      }
    });

    // Compute velocity and internal energy sources.
    par::for_each(particles.all(), [this](PV a) {
      // No source terms for fixed particles.
      if (fixed[a]) return;
      // Apply momentum equation source terms.
      std::apply([a](const auto&... g) { ((dv_dt[a] += g(a)), ...); },
                 momentum_equation_.momentum_sources());
      // Apply energy equation source terms, if necessary.
      if constexpr (has<PV>(du_dt)) {
        std::apply([a](const auto&... q) { ((du_dt[a] += q(a)), ...); },
                   energy_equation_.energy_sources());
      }
      // Compute artificial viscosity switch.
      if constexpr (has<PV>(dalpha_dt)) {
        dalpha_dt[a] = artvisc_.switch_source(a);
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] EquationOfMotion eom_;
  [[no_unique_address]] ContinuityEquation continuity_equation_;
  [[no_unique_address]] MomentumEquation momentum_equation_;
  [[no_unique_address]] EnergyEquation energy_equation_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Viscosity visc_;
  [[no_unique_address]] ArtificialViscosity artvisc_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
