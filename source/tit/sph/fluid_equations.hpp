/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/par/thread.hpp"

#include "tit/sph/TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/density_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<equation_of_state EquationOfState, density_equation DensityEquation,
         kernel Kernel, artificial_viscosity ArtificialViscosity>
class FluidEquations {
private:

  EquationOfState eos_;
  DensityEquation density_equation_;
  Kernel kernel_;
  ArtificialViscosity artvisc_;

public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      meta::Set{fixed, parinfo} | // TODO: fixed should not be here.
#if HARD_DAM_BREAKING
      meta::Set{v_xsph} |
#endif
      meta::Set{h, m, rho, p, r, v, dv_dt} | EquationOfState::required_fields |
      DensityEquation::required_fields | Kernel::required_fields |
      ArtificialViscosity::required_fields;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize fluid equations.
  constexpr explicit FluidEquations(EquationOfState eos = {},
                                    DensityEquation density_equation = {},
                                    Kernel kernel = {},
                                    ArtificialViscosity artvisc = {}) noexcept
      : eos_{std::move(eos)}, density_equation_{std::move(density_equation)},
        kernel_{std::move(kernel)}, artvisc_{std::move(artvisc)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    TIT_PROFILE_SECTION("tit::FluidEquations::init()");
    using PV = ParticleView<ParticleArray>;
    par::static_for_each(particles.views(), [&](PV a) {
      // Initialize particle pressure (and sound speed).
      eos_.compute_pressure(a);
      // Initialize particle artificial viscosity switch value.
      if constexpr (has<PV>(alpha)) alpha[a] = 1.0;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index([[maybe_unused]] ParticleArray& particles,
                       ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void setup_boundary( //
      [[maybe_unused]] ParticleArray& particles,
      [[maybe_unused]] ParticleAdjacency& adjacent_particles) const {
    TIT_PROFILE_SECTION("tit::FluidEquations::setup_boundary()");
#if WITH_WALLS
    using PV = ParticleView<ParticleArray>;
    par::for_each(adjacent_particles._fixed(), [&](auto ia) {
      auto [i, a] = ia;
      auto const search_point = a[r];
      auto const clipped_point = Domain.clamp(search_point);
      auto const r_a = 2 * clipped_point - search_point;
      real_t S = {};
      Mat<real_t, 3> M{};
      constexpr auto SCALE = 3;
      std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
        auto const r_ab = r_a - r[b];
        auto const B_ab = Vec{1.0, r_ab[0], r_ab[1]};
        auto const W_ab = kernel_(r_ab, SCALE * h[a]);
        S += W_ab * m[b] / rho[b];
        M += outer(B_ab, B_ab * W_ab * m[b] / rho[b]);
      });
      auto const fact = ldl(M);
      if (fact) {
        Vec<real_t, 3> e{1.0, 0.0, 0.0};
        auto E = fact->solve(e);
        rho[a] = {};
        v[a] = {};
        std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
          auto const r_ab = r_a - r[b];
          auto const B_ab = Vec{1.0, r_ab[0], r_ab[1]};
          auto W_ab = dot(E, B_ab) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
        });
      } else if (!is_zero(S)) {
        rho[a] = {};
        v[a] = {};
        std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
          auto const r_ab = r_a - r[b];
          auto W_ab = (1 / S) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
        });
      } else {
        goto fail;
      }
      {
        auto const N = normalize(search_point - clipped_point);
        auto const D = norm(r_a - r[a]);
        // drho/dn = rho_0/(cs_0^2)*dot(g,n).
#if EASY_DAM_BREAKING
        constexpr auto rho_0 = 1000.0, cs_0 = 20 * sqrt(9.81 * 0.6);
#elif HARD_DAM_BREAKING
        constexpr auto rho_0 = 1000.0, cs_0 = 120.0;
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
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_density(ParticleArray& particles,
                                 ParticleAdjacency& adjacent_particles) const {
    setup_boundary(particles, adjacent_particles);
    TIT_PROFILE_SECTION("tit::FluidEquations::compute_density()");
    using PV = ParticleView<ParticleArray>;
    // Clean density-related fields.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Density fields.
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
      if constexpr (has<PV>(grad_rho)) grad_rho[a] = {};
      /// Renormalization fields.
      if constexpr (has<PV>(S)) S[a] = {};
      if constexpr (has<PV>(L)) L[a] = {};
    });
    // Compute auxiliary density fields.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      [[maybe_unused]] auto const W_ab = kernel_(a, b);
      [[maybe_unused]] auto const grad_W_ab = kernel_.grad(a, b);
      [[maybe_unused]] auto const V_a = m[a] / rho[a], V_b = m[b] / rho[b];
      /// Update density gradient.
      if constexpr (has<PV>(grad_rho)) {
        auto const grad_flux = rho[b, a] * grad_W_ab;
        grad_rho[a] += V_b * grad_flux, grad_rho[b] += V_a * grad_flux;
      }
      /// Update kernel renormalization coefficient.
      if constexpr (has<PV>(S)) {
        auto const S_flux = W_ab;
        S[a] += V_b * S_flux, S[b] += V_a * S_flux;
      }
      /// Update kernel gradient renormalization matrix.
      if constexpr (has<PV>(L)) {
        auto const L_flux = outer(r[b, a], grad_W_ab);
        L[a] += V_b * L_flux, L[b] += V_a * L_flux;
      }
    });
    // Renormalize fields.
    par::static_for_each(particles.views(), [&](PV a) {
      // Do not renormalize fixed particles.
      if (fixed[a]) return;
      /// Renormalize density (if possible).
      if constexpr (has<PV>(S)) {
        if (!is_zero(S[a])) rho[a] /= S[a];
      }
      /// Renormalize density gradient (if possible).
      if constexpr (has<PV>(L)) {
        auto const fact = ldl(L[a]);
        if (fact) grad_rho[a] = fact->solve(grad_rho[a]);
      }
    });
    // Compute density time derivative. It is computed outside of the upper
    // loop because some artificial viscosities (e.g. δ-SPH) require density
    // gradients (or renormalized density gradients).
    if constexpr (has<PV>(drho_dt)) {
      par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
        auto const [a, b] = ab;
        auto const grad_W_ab = kernel_.grad(a, b);
        auto const V_a = m[a] / rho[a], V_b = m[b] / rho[b];
        /// Compute artificial viscosity density term.
        auto const Psi_ab = artvisc_.density_term(a, b);
        /// Update density time derivative.
        drho_dt[a] += dot(m[b] * v[a, b] + V_b * Psi_ab, grad_W_ab);
        drho_dt[b] -= dot(m[a] * v[b, a] + V_a * Psi_ab, grad_W_ab);
      });
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    TIT_PROFILE_SECTION("tit::FluidEquations::compute_forces()");
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Compute pressure (and sound speed).
      eos_.compute_pressure(a);
      /// Clean velocity-related fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
    });
    // Compute auxiliary velocity fields.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      [[maybe_unused]] auto const W_ab = kernel_(a, b);
      [[maybe_unused]] auto const grad_W_ab = kernel_.grad(a, b);
      [[maybe_unused]] auto const V_a = m[a] / rho[a], V_b = m[b] / rho[b];
      /// Update velocity divergence.
      if constexpr (has<PV>(div_v)) {
        auto const div_flux = dot(v[b, a], grad_W_ab);
        div_v[a] += V_b * div_flux, div_v[b] += V_a * div_flux;
      }
      /// Update velocity curl.
      if constexpr (has<PV>(curl_v)) {
        auto const curl_flux = -cross(v[b, a], grad_W_ab);
        curl_v[a] += V_b * curl_flux, curl_v[b] += V_a * curl_flux;
      }
    });
    // Compute velocity and internal energy time derivatives.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      auto const [a, b] = ab;
      auto const grad_W_ab = kernel_.grad(a, b);
      // Convective updates.
      /// Compute artificial viscosity term.
      auto const Pi_ab = artvisc_.velocity_term(a, b);
      /// Update velocity time derivative.
      auto const v_flux = (-p[a] / pow2(rho[a]) + //
                           -p[b] / pow2(rho[b]) + Pi_ab) *
                          grad_W_ab;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
#if HARD_DAM_BREAKING
      // TODO: Viscosity.
      if constexpr (has<PV>(mu)) {
        // Viscous updates.
        auto const d = dim(r[a]);
        auto const mu_ab = mu.avg(a, b);
        if constexpr (true) {
          /// Laplacian viscosity approach.
          /// Update velocity time derivative.
          // clang-format off
          auto const visc_flux = mu_ab / (rho[a] * rho[b] * norm2(r[a, b])) *
                                 (2 * (d + 2) * dot(v[a, b], r[a, b]) *
                                  grad_W_ab);
          // clang-format on
          dv_dt[a] += m[b] * visc_flux;
          dv_dt[b] -= m[a] * visc_flux;
          if constexpr (has<PV>(u, du_dt)) {
            /// Update internal enegry time derivative.
            du_dt[a] += m[b] * dot(v[a, b], visc_flux);
            du_dt[a] -= m[a] * dot(v[a, b], visc_flux);
          }
        } else {
          // Full stress tensor approach.
          /// Update velocity time derivative.
          // clang-format off
          auto const visc_flux = mu_ab / (rho[a] * rho[b] * norm2(r[a, b])) *
                                 ((d + 2) * dot(v[a, b], r[a, b]) * grad_W_ab +
                                  v[a, b] * dot(r[a, b], grad_W_ab));
          // clang-format on
          dv_dt[a] += m[b] * visc_flux;
          dv_dt[b] -= m[a] * visc_flux;
          if constexpr (has<PV>(u, du_dt)) {
            /// Update internal enegry time derivative.
            du_dt[a] += m[b] * dot(v[a, b], visc_flux);
            du_dt[a] -= m[a] * dot(v[a, b], visc_flux);
          }
        }
      }
#endif
    });
    par::static_for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
      // Compute artificial viscosity switch.
      if constexpr (has<PV>(dalpha_dt)) artvisc_.compute_switch_deriv(a);
    });
  }

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
