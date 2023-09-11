/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>      // IWYU pragma: keep
#include <tuple>       // IWYU pragma: keep
#include <type_traits> // IWYU pragma: keep

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/types.hpp" // IWYU pragma: keep
#include "tit/core/vec.hpp"
#include "tit/par/thread.hpp"

#include "TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/density_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
// TODO: I am not sure whether we should separate "symmetric" and
// "non-symmetric" SPH equations. Basically, implementation of the cases
// are both different and similar at the same time. The entire logic the same,
// although there is a difference is a way we compute interations.
// In symmetric case the best option is to compute pair interactions,
// since the very similar term is added to or subtracted from each particle
// in pair. In non-symmetric case this is no more true, an the most terms are
// generally not similar (mainly because of the different width). So
// implementing loops using unique pair is not sensible any more. (And it
// also requires an additional symmetrization step).
// So first step during this mess would be to split symmetric and
// symmetric adjacency implementations.
template<equation_of_state EquationOfState, density_equation DensityEquation,
         kernel Kernel, artificial_viscosity ArtificialViscosity>
class ClassicSmoothEstimator {
private:

  EquationOfState eos_;
  DensityEquation density_equation_;
  Kernel kernel_;
  ArtificialViscosity artvisc_;

public:

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed, parinfo} | // TODO: fixed should not be here.
#if HARD_DAM_BREAKING
      meta::Set{v_xsph} |
#endif
      meta::Set{h, m, rho, p, r, v, dv_dt} | EquationOfState::required_fields |
      DensityEquation::required_fields | Kernel::required_fields |
      ArtificialViscosity::required_fields;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Initialize particle estimator. */
  constexpr ClassicSmoothEstimator( //
      EquationOfState eos = {}, DensityEquation density_equation = {},
      Kernel kernel = {}, ArtificialViscosity artvisc = {})
      : eos_{std::move(eos)}, density_equation_{std::move(density_equation)},
        kernel_{std::move(kernel)}, artvisc_{std::move(artvisc)} {}

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index(ParticleArray& particles,
                       ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return kernel_.radius(a); });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    par::static_for_each(particles.views(), [&](PV a) {
      // Initialize particle pressure (and sound speed).
      eos_.compute_pressure(a);
      // Inititalize particle width and Omega.
      if constexpr (std::same_as<DensityEquation, GradHSummationDensity>) {
        h[a] = density_equation_.width(a);
        Omega[a] = 1.0;
      }
      // Initialize particle artificial viscosity switch value.
      if constexpr (has<PV>(alpha)) alpha[a] = 1.0;
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Setup boundary particles. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void setup_boundary(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
#if WITH_WALLS
    par::for_each(adjacent_particles.__fixed(), [&](auto ia) {
      auto [i, a] = ia;
      const auto search_point = a[r];
      const auto clipped_point = Domain.clamp(search_point);
      const auto r_a = 2 * clipped_point - search_point;
      real_t S = {};
      Mat<real_t, 3> M{};
      constexpr auto SCALE = 3;
      std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
        const auto r_ab = r_a - r[b];
        const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
        const auto W_ab = kernel_(r_ab, SCALE * h[a]);
        S += W_ab * m[b] / rho[b];
        M += outer(B_ab, B_ab * W_ab * m[b] / rho[b]);
      });
      MatInv inv(M);
      if (inv) {
        Vec<real_t, 3> e{1.0, 0.0, 0.0};
        auto E = inv(e);
        rho[a] = {};
        v[a] = {};
        std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
          const auto r_ab = r_a - r[b];
          const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
          auto W_ab = dot(E, B_ab) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
        });
      } else if (!is_zero(S)) {
        rho[a] = {};
        v[a] = {};
        std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
          const auto r_ab = r_a - r[b];
          auto W_ab = (1 / S) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
        });
      } else {
        goto fail;
      }
      {
        const auto N = normalize(search_point - clipped_point);
        const auto D = norm(r_a - r[a]);
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

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute density-related fields. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_density(ParticleArray& particles,
                                 ParticleAdjacency& adjacent_particles) const {
    setup_boundary(particles, adjacent_particles);
    using PV = ParticleView<ParticleArray>;
    // Calculate density (if density summation is used).
    if constexpr (std::same_as<DensityEquation, SummationDensity>) {
      // Classic density summation.
      par::for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        rho[a] = {};
        std::ranges::for_each(adjacent_particles[a], [&](PV b) {
          const auto W_ab = kernel_(a, b);
          rho[a] += m[b] * W_ab;
        });
      });
    } else if constexpr (std::same_as<DensityEquation, GradHSummationDensity>) {
      // Grad-H density summation.
      par::for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        // Solve zeta(h) = 0 for h, where: zeta(h) = Rho(h) - rho(h),
        // Rho(h) - desired density, defined by the density equation.
        newton_raphson(h[a], [&] {
          rho[a] = {}, Omega[a] = {};
          std::ranges::for_each(adjacent_particles[a], [&](PV b) {
            const auto W_ab = kernel_(a, b, h[a]);
            const auto dW_dh_ab = kernel_.width_deriv(a, b, h[a]);
            rho[a] += m[b] * W_ab, Omega[a] += m[b] * dW_dh_ab;
          });
          const auto [Rho_a, dRho_dh_a] = density_equation_.density(a);
          const auto zeta_a = Rho_a - rho[a];
          const auto dzeta_dh_a = dRho_dh_a - Omega[a];
          Omega[a] = 1.0 - Omega[a] / dRho_dh_a;
          return std::tuple{zeta_a, dzeta_dh_a};
        });
      });
    }
    // Clean density-related fields.
    par::static_for_each(particles.views(), [&](PV a) {
      // Density fields.
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
      if constexpr (has<PV>(grad_rho)) grad_rho[a] = {};
      // Renormalization fields.
      if constexpr (has<PV>(S)) S[a] = {};
      if constexpr (has<PV>(L)) L[a] = {};
    });
    // Compute auxilary density fields.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      [[maybe_unused]] const auto W_ab = kernel_(a, b);
      [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b);
      [[maybe_unused]] const auto V_a = m[a] / rho[a], V_b = m[b] / rho[b];
      // Update density gradient.
      if constexpr (has<PV>(grad_rho)) {
        const auto grad_flux = rho[b, a] * grad_W_ab;
        grad_rho[a] += V_b * grad_flux, grad_rho[b] += V_a * grad_flux;
      }
      // Update kernel renormalization coefficient.
      if constexpr (has<PV>(S)) {
        S[a] += V_b * W_ab, S[b] += V_a * W_ab;
      }
      // Update kernel gradient renormalization matrix.
      if constexpr (has<PV>(L)) {
        const auto L_flux = outer(r[b, a], grad_W_ab);
        L[a] += V_b * L_flux, L[b] += V_a * L_flux;
      }
    });
    // Renormalize fields.
    par::static_for_each(particles.views(), [&](PV a) {
      // Do not renormalize fixed particles.
      if (fixed[a]) return;
      // Renormalize density (if possible).
      if constexpr (has<PV>(S)) {
        if (!is_zero(S[a])) rho[a] /= S[a];
      }
      // Renormalize density gradient (if possible).
      if constexpr (has<PV>(L)) {
        const auto L_a_inv = MatInv{L[a]};
        if (!is_zero(L_a_inv.det())) grad_rho[a] = L_a_inv(grad_rho[a]);
      }
    });
    // Compute density time derivative. It is computed outside of the upper
    // loop because some artificial viscosities (e.g. δ-SPH) require desinty
    // gradients (or renormalized density gradients).
    if constexpr (has<PV>(drho_dt)) {
      par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
        const auto [a, b] = ab;
        const auto grad_W_ab = kernel_.grad(a, b);
        const auto V_a = m[a] / rho[a], V_b = m[b] / rho[b];
        // Compute artificial viscosity diffusive term.
        const auto Psi_ab = artvisc_.density_term(a, b);
        // Update density time derivative.
        // clang-format off
        drho_dt[a] += dot(m[b] * v[a, b] + V_b * Psi_ab,
                          grad_W_ab) / Omega.get(a, 1.0);
        drho_dt[b] -= dot(m[a] * v[b, a] + V_a * Psi_ab,
                          grad_W_ab) / Omega.get(b, 1.0);
        // clang-format on
      });
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute velocity related fields. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::static_for_each(particles.views(), [&](PV a) {
      // Compute pressure (and sound speed).
      eos_.compute_pressure(a);
      // Clean velocity-related fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(u, du_dt)) du_dt[a] = {};
      if constexpr (has<PV>(v_xsph)) v_xsph[a] = {};
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
    });
    // Compute auxilary velocity fields.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      [[maybe_unused]] const auto W_ab = kernel_(a, b);
      [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b);
      [[maybe_unused]] const auto V_a = m[a] / rho[a], V_b = m[b] / rho[b];
      /// Update averaged velocity (XSPH).
      if constexpr (has<PV>(v_xsph)) {
        const auto xsph_flux = v[a, b] / havg(rho[a], rho[b]) * W_ab;
        v_xsph[a] += m[b] * xsph_flux, v_xsph[b] -= m[a] * xsph_flux;
      }
      /// Update velocity divergence.
      if constexpr (has<PV>(div_v)) {
        const auto div_flux = dot(v[b, a], grad_W_ab);
        div_v[a] += V_b * div_flux, div_v[b] += V_a * div_flux;
      }
      /// Update velocity curl.
      if constexpr (has<PV>(curl_v)) {
        const auto curl_flux = -cross(v[b, a], grad_W_ab);
        curl_v[a] += V_b * curl_flux, curl_v[b] += V_a * curl_flux;
      }
    });
    // Compute velocity and internal energy time derivatives.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = [&] {
        // Convective updates.
        /// Compute artificial viscosity diffusive term.
        const auto Pi_ab = artvisc_.velocity_term(a, b);
        if constexpr (has_const<PV>(h)) {
          const auto grad_W_ab = kernel_.grad(a, b);
          /// Update velocity time derivative.
          const auto v_flux = (-p[a] / pow2(rho[a]) + //
                               -p[b] / pow2(rho[b]) + Pi_ab) *
                              grad_W_ab;
          dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
          if constexpr (has<PV>(u, du_dt)) {
            /// Update internal enegry time derivative.
            const auto u_flux = dot(v[b, a], grad_W_ab);
            du_dt[a] += m[b] * (-p[a] / pow2(rho[a]) + Pi_ab) * u_flux;
            du_dt[b] += m[a] * (-p[b] / pow2(rho[b]) + Pi_ab) * u_flux;
          }
          return grad_W_ab;
        } else {
          /// Update velocity time derivative.
          const auto grad_W_aba = kernel_.grad(a, b, h[a]);
          const auto grad_W_abb = kernel_.grad(a, b, h[b]);
          const auto grad_W_ab = avg(grad_W_aba, grad_W_abb);
          /// Update velocity time derivative.
          const auto v_flux = -p[a] / (Omega[a] * pow2(rho[a])) * grad_W_aba +
                              -p[b] / (Omega[b] * pow2(rho[b])) * grad_W_abb +
                              Pi_ab * grad_W_ab;
          dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
          if constexpr (has<PV>(u, du_dt)) {
            /// Update internal enegry time derivative.
            const auto u_flux = dot(v[b, a], grad_W_ab);
            // clang-format off
            du_dt[a] += m[b] * (-p[a] / (Omega[a] * pow2(rho[a])) *
                                dot(v[b, a], grad_W_aba) + Pi_ab * u_flux);
            du_dt[b] += m[a] * (-p[b] / (Omega[b] * pow2(rho[b])) *
                                dot(v[b, a], grad_W_abb) + Pi_ab * u_flux);
            // clang-format on
          }
          return grad_W_ab;
        }
      }();
#if HARD_DAM_BREAKING
      // TODO: Viscosity.
      // -----------------------------------------------------------------------
      if constexpr (has<PV>(mu)) {
        // Viscous updates.
        const auto d = dim(r[a]);
        const auto mu_ab = avg(mu[a], mu[b]);
        if constexpr (true) {
          /// Laplacian viscosity approach.
          /// Update velocity time derivative.
          // clang-format off
          const auto visc_flux = mu_ab / (rho[a] * rho[b] * norm2(r[a, b])) *
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
          const auto visc_flux = mu_ab / (rho[a] * rho[b] * norm2(r[a, b])) *
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
      // -----------------------------------------------------------------------
#endif
#if WITH_WALLS && 0
      // TODO: Lennard-Jones forces.
      // -----------------------------------------------------------------------
      [&](PV a, PV b) {
        if (!fixed[a] && !fixed[b]) return;
        if (fixed[a]) std::swap(a, b);
#if HARD_DAM_BREAKING
        const auto r_0 = 0.01 * 0.7, E_0 = 15.0;
#elif EASY_DAM_BREAKING
        const auto r_0 = 0.6 / 80.0 * 0.7, E_0 = 9.81 * 0.6;
#endif
        const auto r_ab = norm(r[a, b]);
        if (r_ab < r_0) {
          auto P1 = 4, P2 = 2;
          dv_dt[a] += (E_0 / m[a] / pow2(r_ab)) *
                      (pow(r_0 / r_ab, P1) - pow(r_0 / r_ab, P2)) * r[a, b];
        }
      }(a, b);
      // -----------------------------------------------------------------------
#endif
    });
    par::static_for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
#if WITH_WALLS && 0
      {
#if HARD_DAM_BREAKING
        const auto r_0 = 0.01 * 0.6, E_0 = 15.0;
#elif EASY_DAM_BREAKING
        const auto r_0 = 0.6 / 80.0 * 0.25, E_0 = 9.81 * 0.6;
#endif
        const auto clipped_point = Domain.proj(r[a]);
        const auto r_ab = norm(r[a] - clipped_point);
        if (r_ab < r_0) {
          auto P1 = 4, P2 = 2;
          dv_dt[a] += (E_0 / m[a] / pow2(r_ab)) *
                      (pow(r_0 / r_ab, P1) - pow(r_0 / r_ab, P2)) *
                      (r[a] - clipped_point);
        }
      }
#endif
      // Compute artificial viscosity switch.
      if constexpr (has<PV>(dalpha_dt)) artvisc_.compute_switch_deriv(a);
    });
  }

}; // class ClassicSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
