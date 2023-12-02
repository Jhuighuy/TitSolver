/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <tuple>

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/thread.hpp"
#include "tit/sph/TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/density_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with density summation.
\******************************************************************************/
template<equation_of_state EquationOfState, density_equation DensityEquation,
         kernel Kernel, artificial_viscosity ArtificialViscosity>
class CompressibleFluidEquations {
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

  /** Initialize fluid equations. */
  constexpr explicit CompressibleFluidEquations(
      EquationOfState eos = {}, DensityEquation density_equation = {},
      Kernel kernel = {}, ArtificialViscosity artvisc = {}) noexcept
      : eos_{std::move(eos)}, density_equation_{std::move(density_equation)},
        kernel_{std::move(kernel)}, artvisc_{std::move(artvisc)} {}

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    par::static_for_each(particles.views(), [&](PV a) {
      // Initialize particle pressure (and sound speed).
      eos_.compute_pressure(a);
      // Initialize particle width and Omega.
      if constexpr (std::same_as<DensityEquation, GradHSummationDensity>) {
        h[a] = density_equation_.width(a);
        Omega[a] = 1.0;
      }
      // Initialize particle artificial viscosity switch value.
      if constexpr (has<PV>(alpha)) alpha[a] = 1.0;
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index([[maybe_unused]] ParticleArray& particles,
                       ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return kernel_.radius(a); });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Setup boundary particles. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void setup_boundary( //
      [[maybe_unused]] ParticleArray& particles,
      [[maybe_unused]] ParticleAdjacency& adjacent_particles) const {
#if WITH_WALLS
    using PV = ParticleView<ParticleArray>;
    par::for_each(adjacent_particles._fixed(), [&](auto ia) {
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
    // Calculate density.
    if constexpr (std::same_as<DensityEquation, SummationDensity>) {
      /// Classic density summation.
      par::for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        rho[a] = {};
        std::ranges::for_each(adjacent_particles[a], [&](PV b) {
          const auto W_ab = kernel_(a, b);
          rho[a] += m[b] * W_ab;
        });
      });
    } else if constexpr (std::same_as<DensityEquation, GradHSummationDensity>) {
      /// Grad-H density summation.
      par::for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        /// Solve `zeta(h) = 0` for `h`, where: `zeta(h) = Rho(h) - rho(h)`,
        /// `Rho(h)` - desired density, defined by the density equation.
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
    // Compute renormalization fields.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Clean renormalization fields.
      if constexpr (has<PV>(S)) S[a] = {};
      if constexpr (has<PV>(L)) L[a] = {};
      /// Compute renormalization fields.
      std::ranges::for_each(adjacent_particles[a], [&](PV b) {
        [[maybe_unused]] const auto W_ab = kernel_(a, b, h[a]);
        [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b, h[a]);
        [[maybe_unused]] const auto V_b = m[b] / rho[b];
        /// Update kernel renormalization coefficient.
        if constexpr (has<PV>(S)) S[a] += V_b * W_ab;
        /// Update kernel gradient renormalization matrix.
        if constexpr (has<PV>(L)) L[a] += V_b * outer(r[b, a], grad_W_ab);
      });
      /// Finalize kernel renormalization coefficient.
      if constexpr (has<PV>(S)) {
        if (is_zero(S[a])) S[a] = 1.0;
        else S[a] = inverse(S[a]);
      }
      /// Finalize kernel gradient renormalization matrix.
      if constexpr (has<PV>(L)) {
        const auto invL_a = MatInv{L[a]};
        if (is_zero(invL_a.det())) L[a] = 1.0;
        else L[a] = invL_a();
      }
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute velocity related fields. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Compute velocity derivative fields.
    par::static_for_each(particles.views(), [&](PV a) {
      /// Compute pressure (and sound speed).
      eos_.compute_pressure(a);
      /// Clean velocity derivative fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(u, du_dt)) du_dt[a] = {};
      if constexpr (has<PV>(grad_v)) {
        grad_v[a] = {};
      } else {
        if constexpr (has<PV>(div_v)) div_v[a] = {};
        if constexpr (has<PV>(curl_v)) curl_v[a] = {};
      }
      /// Compute velocity derivative fields.
      std::ranges::for_each(adjacent_particles[a], [&](PV b) {
        [[maybe_unused]] const auto grad_W_ab = kernel_.grad(a, b, h[a]);
        [[maybe_unused]] const auto V_b = m[b] / rho[b];
        if constexpr (has<PV>(grad_v)) {
          /// Update velocity gradient.
          grad_v[a] += outer(v[b, a], grad_W_ab);
        } else {
          /// Update velocity divergence.
          if constexpr (has<PV>(div_v)) {
            div_v[a] += V_b * dot(v[b, a], grad_W_ab);
          }
          /// Update velocity curl.
          if constexpr (has<PV>(curl_v)) {
            curl_v[a] += V_b * -cross(v[b, a], grad_W_ab);
          }
        }
      });
      /// Renormalize velocity gradient.
      if constexpr (has<PV>(L, grad_v)) grad_v[a] = L[a] * grad_v[a];
      /// Compute velocity divergence from gradient.
      if constexpr (has<PV>(div_v, grad_v)) div_v[a] = tr(grad_v[a]);
      /// Compute velocity curl from gradient.
      if constexpr (has<PV>(curl_v, grad_v)) curl_v[a] = grad2curl(grad_v[a]);
    });
    // Compute velocity and internal energy time derivatives.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_aba = kernel_.grad(a, b, h[a]);
      const auto grad_W_abb = kernel_.grad(a, b, h[b]);
      /// Compute artificial viscosity term.
      const auto Pi_ab = artvisc_.velocity_term(a, b);
      /// Update velocity time derivative.
      // clang-format off
      const auto v_flux_a = (-p[a] / (Omega.get(a, 1.0) * pow2(rho[a])) +
                             0.5 * Pi_ab) * grad_W_aba;
      const auto v_flux_b = (-p[b] / (Omega.get(b, 1.0) * pow2(rho[b])) +
                             0.5 * Pi_ab) * grad_W_abb;
      // clang-format on
      const auto v_flux = v_flux_a + v_flux_b;
      dv_dt[a] += m[b] * v_flux, dv_dt[b] -= m[a] * v_flux;
      if constexpr (has<PV>(u, du_dt)) {
        /// Compute artificial conductivity term.
        const auto alpha_u_ = 1.0;
        const auto v_sig_ab = sqrt(abs(p[a, b]) / rho.avg(a, b));
        const auto Lambda_ab = alpha_u_ * v_sig_ab * u[a, b] * r[a, b] /
                               norm(r[a, b]) / rho.avg(a, b);
        const auto Lambda_flux = dot(Lambda_ab, avg(grad_W_aba, grad_W_abb));
        /// Update internal enegry time derivative.
        du_dt[a] += m[b] * (dot(v_flux_a, v[b, a]) + Lambda_flux);
        du_dt[b] += m[a] * (dot(v_flux_b, v[b, a]) - Lambda_flux);
      }
    });
    par::static_for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
      /// Compute artificial viscosity switch time.
      if constexpr (has<PV>(dalpha_dt)) artvisc_.compute_switch_deriv(a);
    });
  }

}; // class CompressibleFluidEquations

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
