/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>

#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/TitParticle.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_motion.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation, that
/// use Godunov's method.
template<equation_of_motion EquationOfMotion,
         continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         energy_equation EnergyEquation,
         equation_of_state EquationOfState,
         viscosity Viscosity,
         kernel Kernel>
class GodunovFluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      EquationOfMotion::required_fields |   //
      ContinuityEquation::required_fields | //
      MomentumEquation::required_fields |   //
      EnergyEquation::required_fields |     //
      EquationOfState::required_fields |    //
      Viscosity::required_fields |          //
      Kernel::required_fields |             //
      meta::Set{fixed, parinfo} |           // TODO: fixed should not be here.
      meta::Set{h, m, rho, drho_dt, p, cs, r, v, dv_dt, u, du_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      EquationOfMotion::modified_fields |   //
      ContinuityEquation::modified_fields | //
      MomentumEquation::modified_fields |   //
      EnergyEquation::modified_fields |     //
      EquationOfState::modified_fields |    //
      Viscosity::modified_fields |          //
      Kernel::modified_fields |             //
      meta::Set{p, rho, drho_dt, grad_rho, v, dv_dt, div_v, curl_v, u, du_dt} |
      meta::Set{cs, S, L};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  constexpr explicit GodunovFluidEquations(EquationOfMotion eom,
                                           ContinuityEquation continuity,
                                           MomentumEquation momentum,
                                           EnergyEquation energy,
                                           EquationOfState eos,
                                           Viscosity viscosity,
                                           Kernel kernel) noexcept
      : eom_{std::move(eom)}, continuity_{std::move(continuity)},
        momentum_{std::move(momentum)}, energy_{std::move(energy)},
        eos_{std::move(eos)}, viscosity_{std::move(viscosity)},
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_array<required_fields> ParticleArray>
  constexpr void init(ParticleArray& /*particles*/) const {
    // Nothing to do.
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
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    // Clean density-related fields.
    par::for_each(particles.views(), [&](PV a) {
      // Density fields.
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_forces(ParticleMesh& mesh,
                                ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::for_each(particles.views(), [&](PV a) {
      /// Compute pressure (and sound speed).
      p[a] = eos_.pressure(a);
      /// Clean velocity-related fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(du_dt)) du_dt[a] = {};
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
    });
    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);
      /// Solve Riemann problem.
      const auto e_ab = -normalize(r[a, b]);
      const auto U_a = dot(v[a], e_ab);
      const auto U_b = dot(v[b], e_ab);
      const auto U_ab = (rho[a] * U_a + rho[b] * U_b) / (rho[a] + rho[b]);
      const auto p_ab = (rho[a] * p[a] + rho[b] * p[b]) / (rho[a] + rho[b]);
      const auto v_ab = (rho[a] * v[a] + rho[b] * v[b]) / (rho[a] + rho[b]);
      const auto rho_ab = avg(rho[a], rho[b]);
      const auto U_ast = U_ab + 0.5 * (p[a] - p[b]) / (rho_ab * cs[a]);
      const auto v_ast = U_ast * e_ab + (v_ab - U_ab * e_ab);
      const auto beta = std::min(cs[a], 3 * std::max(U_a - U_b, 0.0));
      const auto p_ast =
          p_ab + 0.5 * rho[a] * rho[b] / rho_ab * cs[a] * beta * (U_a - U_b);
      /// Update density time derivative.
      drho_dt[a] += 2.0 * rho[a] * m[b] / rho[b] * dot(v[a] - v_ast, grad_W_ab);
      drho_dt[b] -= 2.0 * rho[b] * m[a] / rho[a] * dot(v[b] - v_ast, grad_W_ab);
      /// Update velocity time derivative.
      const auto v_flux = -2.0 * p_ast / (rho[a] * rho[b]) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
    });
    par::for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] EquationOfMotion eom_;
  [[no_unique_address]] ContinuityEquation continuity_;
  [[no_unique_address]] MomentumEquation momentum_;
  [[no_unique_address]] EnergyEquation energy_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Viscosity viscosity_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
