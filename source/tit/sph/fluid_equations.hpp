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

#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/motion_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<motion_equation MotionEquation,
         continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         energy_equation EnergyEquation,
         equation_of_state EquationOfState,
         kernel Kernel>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =   //
      MotionEquation::required_fields |     //
      ContinuityEquation::required_fields | //
      MomentumEquation::required_fields |   //
      EnergyEquation::required_fields |     //
      EquationOfState::required_fields |    //
      Kernel::required_fields |
      meta::Set{parinfo} | // TODO: parinfo should not be here.
      meta::Set{h, m, r, rho, p, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      MotionEquation::modified_fields |         //
      ContinuityEquation::modified_fields |     //
      MomentumEquation::modified_fields |       //
      EnergyEquation::modified_fields |         //
      EquationOfState::modified_fields |        //
      Kernel::modified_fields |                 //
      meta::Set{rho, drho_dt, grad_rho, S, L} | //
      meta::Set{p, v, dv_dt, div_v, curl_v} |   //
      meta::Set{u, du_dt};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param motion_equation     Motion equation.
  /// @param continuity_equation Continuity equation.
  /// @param momentum_equation   Momentum equation.
  /// @param energy_equation     Energy equation.
  /// @param equation_of_state   Equation of state.
  /// @param kernel              Kernel.
  constexpr explicit FluidEquations(MotionEquation motion_equation,
                                    ContinuityEquation continuity_equation,
                                    MomentumEquation momentum_equation,
                                    EnergyEquation energy_equation,
                                    EquationOfState eos,
                                    Kernel kernel) noexcept
      : motion_equation_{std::move(motion_equation)},
        continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        energy_equation_{std::move(energy_equation)}, //
        eos_{std::move(eos)},                         //
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_array<required_fields> ParticleArray>
  void init([[maybe_unused]] ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::init()");
    using PV = ParticleView<ParticleArray>;

    // Initialize particle artificial viscosity switch value.
    if constexpr (has<PV>(alpha)) {
      par::for_each(particles.all(), [](PV a) { alpha[a] = 1.0; });
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.update(particles, [this](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void setup_boundary(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::setup_boundary()");
    using PV = ParticleView<ParticleArray>;
    using Num = particle_num_t<ParticleArray>;
    static constexpr auto Dim = particle_dim_v<ParticleArray>;

    // Interpolate the field values on the boundary.
    par::for_each(particles.fixed(), [this, &mesh](PV b) {
      /// @todo Once we have a proper geometry library, we should use
      ///       here and clean up the code.
      const auto& search_point = r[b];
      const auto clipped_point = Domain.clamp(search_point);
      const auto r_ghost = 2 * clipped_point - search_point;
      const auto N = normalize(search_point - clipped_point);
      const auto D = norm(r_ghost - r[b]);

      // Compute the interpolation weights, both for the constant and
      // linear interpolations.
      Num S{};
      Mat<Num, Dim + 1> M{};
      const auto h_ghost = RADIUS_SCALE * h[b];
      for (const PV a : mesh.fixed_interp(b)) {
        const auto r_delta = r_ghost - r[a];
        const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
        const auto W_delta = kernel_(r_delta, h_ghost);
        S += W_delta * m[a] / rho[a];
        M += outer(B_delta, B_delta * W_delta * m[a] / rho[a]);
      }

      if (const auto fact = ldl(M); fact) {
        // Linear interpolation succeeds, use it.
        rho[b] = {};
        v[b] = {};
        if constexpr (has<PV>(u)) u[b] = {};
        const auto E = fact->solve(unit<0>(M[0]));
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
          const auto W_delta = dot(E, B_delta) * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
          if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
        }
      } else if (!is_tiny(S)) {
        // Constant interpolation succeeds, use it.
        rho[b] = {};
        v[b] = {};
        if constexpr (has<PV>(u)) u[b] = {};
        const auto E = inverse(S);
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto W_delta = E * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
          if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
        }
      } else {
        // Both interpolations fail, leave the particle as it is.
        return;
      }

      // Compute the density at the boundary.
      // drho/dn = rho_0/(cs_0^2)*dot(g,n).
      constexpr auto rho_0 = 1000.0;
      constexpr auto cs_0 = 20 * sqrt(9.81 * 0.6);
      constexpr auto G = Vec{0.0, -9.81};
      rho[b] += D * rho_0 / pow2(cs_0) * dot(G, N);

      // Compute the velocity at the boundary (slip wall boundary condition).
      const auto Vn = dot(v[b], N) * N;
      const auto Vt = v[b] - Vn;
      v[b] = Vt - Vn;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_density(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up continuity equation fields and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up continuity equation fields.
      drho_dt[a] = {};
      if constexpr (has<PV>(grad_rho)) grad_rho[a] = {};
      if constexpr (has<PV>(S)) S[a] = {};
      if constexpr (has<PV>(L)) L[a] = {};

      // Apply continuity equation source terms.
      std::apply([a](const auto&... f) { ((drho_dt[a] += f(a)), ...); },
                 continuity_equation_.mass_sources());
    });

    // Compute density gradient and renormalization fields.
    if constexpr (has<PV>(grad_rho) || has<PV>(S) || has<PV>(L)) {
      par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
        const auto [a, b] = ab;
        const auto V_a = m[a] / rho[a];
        const auto V_b = m[b] / rho[b];
        [[maybe_unused]] const auto W_ab = kernel_(a, b);
        [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b);

        // Update density gradient.
        if constexpr (has<PV>(grad_rho)) {
          const auto grad_flux = rho[b, a] * grad_W_ab;
          grad_rho[a] += V_b * grad_flux;
          grad_rho[b] += V_a * grad_flux;
        }

        // Update kernel renormalization coefficient.
        if constexpr (has<PV>(S)) {
          const auto S_flux = W_ab;
          S[a] += V_b * S_flux;
          S[b] += V_a * S_flux;
        }

        // Update kernel gradient renormalization matrix.
        if constexpr (has<PV>(L)) {
          const auto L_flux = outer(r[b, a], grad_W_ab);
          L[a] += V_b * L_flux;
          L[b] += V_a * L_flux;
        }
      });
    }

    // Renormalize fields.
    if constexpr (has<PV>(S) || has<PV>(L)) {
      par::for_each(particles.fluid(), [](PV a) {
        // Renormalize density, if possible.
        if constexpr (has<PV>(S)) {
          if (!is_tiny(S[a])) rho[a] /= S[a];
        }

        // Renormalize density gradient, if possible.
        if constexpr (has<PV>(L, grad_rho)) {
          if (const auto fact = ldl(L[a]); fact) {
            grad_rho[a] = fact->solve(grad_rho[a]);
          }
        }
      });
    }

    // Compute density time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);

      // Update density time derivative.
      const auto Psi_ab =
          momentum_equation_.artificial_viscosity().density_term(a, b);
      drho_dt[a] -= m[b] * dot(v[b, a] - Psi_ab / rho[b], grad_W_ab);
      drho_dt[b] -= m[a] * dot(v[b, a] + Psi_ab / rho[a], grad_W_ab);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_forces()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up momentum and energy equation fields, compute pressure,
    // sound speed and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up momentum and energy equation fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
      if constexpr (has<PV>(du_dt)) du_dt[a] = {};

      // Apply source terms.
      std::apply(
          [a](const auto&... g) {
            ((dv_dt[a] += g(a)), ...);
            if constexpr (has<PV>(du_dt)) ((du_dt[a] += dot(g(a), v[a])), ...);
          },
          momentum_equation_.momentum_sources());
      if constexpr (has<PV>(du_dt)) {
        std::apply([a](const auto&... q) { ((du_dt[a] += q(a)), ...); },
                   energy_equation_.energy_sources());
      }

      // Compute pressure and sound speed.
      p[a] = eos_.pressure(a);
      if constexpr (has<PV>(cs)) cs[a] = eos_.sound_speed(a);
    });

    // Compute velocity divergence and curl.
    // Those fields may be required by the artificial viscosity.
    if constexpr (has<PV>(div_v) || has<PV>(curl_v)) {
      par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
        const auto [a, b] = ab;
        const auto V_a = m[a] / rho[a];
        const auto V_b = m[b] / rho[b];
        const auto grad_W_ab = kernel_.grad(a, b);

        // Update velocity divergence.
        if constexpr (has<PV>(div_v)) {
          const auto div_flux = dot(v[b, a], grad_W_ab);
          div_v[a] += V_b * div_flux;
          div_v[b] += V_a * div_flux;
        }

        // Update velocity curl.
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
      const auto Pi_ab =
          momentum_equation_.viscosity()(a, b) +
          momentum_equation_.artificial_viscosity().velocity_term(a, b);
      const auto v_flux = (-P_a - P_b + Pi_ab) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;

      // Update internal energy time derivative.
      if constexpr (has<PV>(du_dt)) {
        const auto Q_ab = energy_equation_.heat_conductivity()(a, b);
        du_dt[a] -= m[b] * dot((P_a - Pi_ab / 2) * v[b, a] - Q_ab, grad_W_ab);
        du_dt[b] -= m[a] * dot((P_b - Pi_ab / 2) * v[b, a] + Q_ab, grad_W_ab);
      }
    });

    // Compute artificial viscosity switch.
    if constexpr (has<PV>(dalpha_dt)) {
      par::for_each(particles.fluid(), [this](PV a) {
        dalpha_dt[a] =
            momentum_equation_.artificial_viscosity().switch_source(a);
      });
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] MotionEquation motion_equation_;
  [[no_unique_address]] ContinuityEquation continuity_equation_;
  [[no_unique_address]] MomentumEquation momentum_equation_;
  [[no_unique_address]] EnergyEquation energy_equation_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
