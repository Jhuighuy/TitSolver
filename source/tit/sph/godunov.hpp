/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <tuple>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/density_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The particle estimator with a fixed kernel width.
// TODO: I am not sure whether we should separate "symmetric" and
// "non-symmetric" SPH equations. Basically, implementation of the cases
// are both different and similar at the same time. The entire logic the same,
// although there is a difference is a way we compute interactions.
// In symmetric case the best option is to compute pair interactions,
// since the very similar term is added to or subtracted from each particle
// in pair. In non-symmetric case this is no more true, an the most terms are
// generally not similar (mainly because of the different width). So
// implementing loops using unique pair is not sensible any more. (And it
// also requires an additional symmetrization step).
// So first step during this mess would be to split symmetric and
// symmetric adjacency implementations.
template<equation_of_state EquationOfState,
         density_equation DensityEquation,
         kernel Kernel,
         artificial_viscosity ArtificialViscosity>
class GodunovFluidEquations {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      meta::Set{fixed, parinfo} | // TODO: fixed should not be here.
#if HARD_DAM_BREAKING
      meta::Set{v_xsph} |
#endif
      meta::Set{h, m, rho, p, cs, r, v, dv_dt} |
      EquationOfState::required_fields | DensityEquation::required_fields |
      Kernel::required_fields | ArtificialViscosity::required_fields;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize fluid equations.
  constexpr GodunovFluidEquations(EquationOfState eos = {},
                                  DensityEquation density_equation = {},
                                  Kernel kernel = {},
                                  ArtificialViscosity artvisc = {}) noexcept
      : eos_{std::move(eos)}, density_equation_{std::move(density_equation)},
        kernel_{std::move(kernel)}, artvisc_{std::move(artvisc)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    par::static_for_each(particles.all(), [this](PV a) {
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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index(ParticleArray& /*particles*/,
                       ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void setup_boundary(
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
      const auto fact = ldl(M);
      if (fact) {
        Vec<real_t, 3> e{1.0, 0.0, 0.0};
        auto E = fact->solve(e);
        rho[a] = {};
        v[a] = {};
        std::ranges::for_each(adjacent_particles[nullptr, i], [&](PV b) {
          const auto r_ab = r_a - r[b];
          const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
          auto W_ab = dot(E, B_ab) * kernel_(r_ab, SCALE * h[a]);
          rho[a] += m[b] * W_ab;
          v[a] += m[b] / rho[b] * v[b] * W_ab;
        });
      } else if (!is_tiny(S)) {
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
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_density(ParticleArray& particles,
                                 ParticleAdjacency& adjacent_particles) const {
    setup_boundary(particles, adjacent_particles);
    using PV = ParticleView<ParticleArray>;
    // Clean density-related fields.
    par::static_for_each(particles.all(), [](PV a) {
      // Density fields.
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Prepare velocity-related fields.
    par::static_for_each(particles.all(), [this](PV a) {
      /// Compute pressure (and sound speed).
      eos_.compute_pressure(a);
      /// Clean velocity-related fields.
      dv_dt[a] = {};
      if constexpr (has<PV>(du_dt)) du_dt[a] = {};
      if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
    });
    // Compute velocity and internal energy time derivatives.
    par::block_for_each(adjacent_particles.block_pairs(), [&](auto ab) {
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
    par::static_for_each(particles.all(), [](PV a) {
      if (fixed[a]) return;
#if WITH_GRAVITY
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] DensityEquation density_equation_;
  [[no_unique_address]] Kernel kernel_;
  [[no_unique_address]] ArtificialViscosity artvisc_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
