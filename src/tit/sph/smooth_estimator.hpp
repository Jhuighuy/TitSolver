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
#include <ranges>      // IWYU pragma: keep
#include <tuple>       // IWYU pragma: keep
#include <type_traits> // IWYU pragma: keep

#include "tit/core/bbox.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/types.hpp" // IWYU pragma: keep
#include "tit/core/vec.hpp"

#include "TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/equation_of_state.hpp" // IWYU pragma: keep
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<class EquationOfState, class Kernel = QuarticSplineKernel,
         class ArtificialViscosity = MorrisMonaghanArtificialViscosity<>>
  requires std::is_object_v<EquationOfState> && std::is_object_v<Kernel> &&
           std::is_object_v<ArtificialViscosity>
class ClassicSmoothEstimator {
private:

  EquationOfState _eos;
  Kernel _kernel;
  ArtificialViscosity _artvisc;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed} | // TODO: fixed should not be here.
      meta::Set{h, m, rho, p, r, v, dv_dt} |
      meta::Set{drho_dt} | // TODO: move me to appropiate place.
      EquationOfState::required_fields | Kernel::required_fields |
      ArtificialViscosity::required_fields;

  /** Initialize particle estimator. */
  constexpr ClassicSmoothEstimator( //
      EquationOfState eos = {}, Kernel kernel = {},
      ArtificialViscosity artvisc = {})
      : _eos{std::move(eos)}, _kernel{std::move(kernel)},
        _artvisc{std::move(artvisc)} {}

  template<class ParticleArray, class ParticleAdjacency>
  constexpr auto index(ParticleArray& particles,
                       ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    adjacent_particles.build([&](PV a) { return _kernel.radius(a); });
  }

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    std::ranges::for_each(particles.views(), [&](auto a) {
      // Init particle pressure (and sound speed).
      _eos.compute_pressure(a);
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute density, pressure (and sound speed). */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_density(ParticleArray& particles,
                                 ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Clean fields that would be calculated.
    size_t xxx = 0;
    std::ranges::for_each(particles.views(), [&](PV a) {
      { // Density fields.
        if constexpr (has<PV>(drho_dt)) drho_dt[a] = {};
        else if (!fixed[a]) rho[a] = {};
        if constexpr (has<PV>(grad_rho)) grad_rho[a] = {};
      }
      { // Velocity fields.
        if constexpr (has<PV>(div_v)) div_v[a] = {};
        if constexpr (has<PV>(curl_v)) curl_v[a] = {};
        if constexpr (has<PV>(v_xsph)) v_xsph[a] = {};
      }
      { // And other fields.
        if constexpr (has<PV>(L)) L[a] = {};
      }
#if 1
      // -----------------------------------------------------------------------
      if (fixed[a]) {
        const auto search_point = a[r];
        const auto clipped_point = Domain.clip(search_point);
        real_t S = {};
        Mat<real_t, 3> M{};
        std::ranges::for_each(adjacent_particles[nullptr, xxx], [&](PV b) {
          if (fixed[b]) return;
          const auto r_a = 2 * clipped_point - search_point;
          const auto r_ab = r_a - r[b];
          const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
          const auto W_ab = _kernel(r_ab, 2 * h[a]);
          S += W_ab * m[b] / rho[b];
          M += outer(B_ab, B_ab * W_ab * m[b] / rho[b]);
        });
        MatInv inv(M);
        if (inv) {
          Vec<real_t, 3> e{1.0, 0.0, 0.0};
          auto E = inv(e);
          rho[a] = {};
          v[a] = {};
          std::ranges::for_each(adjacent_particles[nullptr, xxx], [&](PV b) {
            if (fixed[b]) return;
            const auto r_a = 2 * clipped_point - search_point;
            const auto r_ab = r_a - r[b];
            const auto B_ab = Vec{1.0, r_ab[0], r_ab[1]};
            auto W_ab = dot(E, B_ab) * _kernel(r_ab, 2 * h[a]);
            rho[a] += m[b] * W_ab;
            v[a] += m[b] / rho[b] * v[b] * W_ab;
          });
        } else if (!is_zero(S)) {
          rho[a] = {};
          v[a] = {};
          std::ranges::for_each(adjacent_particles[nullptr, xxx], [&](PV b) {
            if (fixed[b]) return;
            const auto r_a = 2 * clipped_point - search_point;
            const auto r_ab = r_a - r[b];
            auto W_ab = (1 / S) * _kernel(r_ab, 2 * h[a]);
            rho[a] += m[b] * W_ab;
            v[a] += m[b] / rho[b] * v[b] * W_ab;
          });
        } else {
          goto fail;
        }
#if 0
        const auto N = normalize(clipped_point - search_point);
        auto V = v[a]; // V = (V)
        auto Vn = dot(V, N) * N;
        auto Vt = V - Vn;
        v[a] = Vt - Vn;
#else
        v[a] *= -1;
#endif
      fail:
        ++xxx;
      }
      // -----------------------------------------------------------------------
#endif
    });
    // Compute updates for fields.
    std::ranges::for_each(adjacent_particles.pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      if (a == b) return;
      if (fixed[a] && fixed[b]) return;
      [[maybe_unused]] const auto W_ab = _kernel(a, b);
      [[maybe_unused]] const auto grad_W_ab = _kernel.grad(a, b);
      { // Density fields.
        /// Update density (or density time derivative).
        if constexpr (has<PV>(drho_dt)) {
          // Continuity equation approach.
          // Compute artificial viscosity diffusive term.
          const auto Psi_ab = _artvisc.density_term(a, b);
          // Update density time derivative.
          drho_dt[a] += m[b] * dot(v[a, b] + Psi_ab / rho[b], grad_W_ab);
          drho_dt[b] -= m[a] * dot(v[b, a] + Psi_ab / rho[a], grad_W_ab);
        } else {
          // Density summation approach.
          // TODO: extract density summation into the separate loop.
          if (!fixed[a]) rho[a] += m[b] * W_ab;
          if (!fixed[b]) rho[b] += m[a] * W_ab;
        }
        /// Update density gradient.
        if constexpr (has<PV>(grad_rho)) {
          static_assert(has<PV>(drho_dt),
                        "Can't compute rho and grad rho at the same time.");
          const auto grad_flux = rho[b, a] / (rho[a] * rho[b]) * grad_W_ab;
          grad_rho[a] += m[b] * grad_flux;
          grad_rho[b] += m[a] * grad_flux;
        }
      }
      { // Velocity fields.
        /// Update averaged velocity (XSPH).
        if constexpr (has<PV>(v_xsph)) {
          const auto xsph_flux = v[a, b] / havg(rho[a], rho[b]) * W_ab;
          v_xsph[a] += m[b] * xsph_flux;
          v_xsph[b] -= m[a] * xsph_flux;
        }
        /// Update velocity divergence.
        if constexpr (has<PV>(div_v)) {
          // clang-format off
          const auto div_flux = dot(v[a] / pow2(rho[a]) +
                                    v[b] / pow2(rho[b]), grad_W_ab);
          // clang-format on
          div_v[a] += m[b] * div_flux;
          div_v[b] -= m[a] * div_flux;
        }
        /// Update velocity curl.
        if constexpr (has<PV>(curl_v)) {
          // clang-format off
          const auto curl_flux = -cross(v[a] / pow2(rho[a]) +
                                        v[b] / pow2(rho[b]), grad_W_ab);
          // clang-format on
          curl_v[a] += m[b] * curl_flux;
          curl_v[b] -= m[a] * curl_flux;
        }
      }
      { // And other fields.
        /// Update renormalization matrix.
        if constexpr (has<PV>(L)) {
          const auto L_flux = outer(r[b, a] / (rho[a] * rho[b]), grad_W_ab);
          L[a] += m[b] * L_flux;
          L[b] += m[a] * L_flux;
        }
      }
    });
    // Finalize fields.
    std::ranges::for_each(particles.views(), [&](PV a) {
      { // Density fields.
        /// Finalize density.
        if constexpr (!has<PV>(drho_dt)) {
          if (!fixed[a]) {
            const auto W_aa = _kernel(a, a);
            rho[a] += m[a] * W_aa;
          }
        }
        /// Compute pressure (and sound speed).
        _eos.compute_pressure(a);
        /// Finalize density gradient.
        if constexpr (has<PV>(grad_rho)) grad_rho[a] *= rho[a];
      }
      { // Velocity fields.
        /// Finalize velocity divergence and curl.
        if constexpr (has<PV>(div_v)) div_v[a] *= rho[a];
        if constexpr (has<PV>(curl_v)) curl_v[a] *= rho[a];
      }
      { /// And other fields.
        // Finalize renormalization matrix.
        if constexpr (has<PV>(L)) {
          L[a] *= rho[a];
          auto inv = MatInv{L[a]};
          if (inv) L[a] = inv();
          else L[a] = 1.0;
        }
      }
    });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Compute acceleration and thermal heating. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void compute_forces(ParticleArray& particles,
                                ParticleAdjacency& adjacent_particles) const {
    using PV = ParticleView<ParticleArray>;
    // Clean fields that would be calculated.
    std::ranges::for_each(particles.views(), [&](PV a) {
      dv_dt[a] = {};
      if constexpr (has<PV>(eps, deps_dt)) deps_dt[a] = {};
    });
    // Compute updates for fields.
    std::ranges::for_each(adjacent_particles.pairs(), [&](auto ab) {
      const auto [a, b] = ab;
      if (a == b) return;
      if (fixed[a] && fixed[b]) return;
      const auto grad_W_ab = _kernel.grad(a, b);
      { // Convective updates.
        /// Compute artificial viscosity diffusive term.
        const auto Pi_ab = _artvisc.velocity_term(a, b);
        /// Update velocity time derivative.
        // clang-format off
        const auto conv_flux = -(p[a] / pow2(rho[a]) +
                                 p[b] / pow2(rho[b]) + Pi_ab) * grad_W_ab;
        // clang-format on
        dv_dt[a] += m[b] * conv_flux;
        dv_dt[b] -= m[a] * conv_flux;
        if constexpr (has<PV>(eps, deps_dt)) {
          /// Update internal enegry time derivative.
          // clang-format off
          deps_dt[a] += m[b] * (p[a] / pow2(rho[a]) + Pi_ab) *
                               dot(v[a, b], grad_W_ab);
          deps_dt[b] += m[a] * (p[b] / pow2(rho[b]) + Pi_ab) *
                               dot(v[a, b], grad_W_ab);
          // clang-format on
        }
      }
    });
    std::ranges::for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
      // Compute artificial viscosity switch.
      if constexpr (has<PV>(dalpha_dt)) _artvisc.compute_switch_deriv(a);
    });
  }

}; // class ClassicSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if 0
/******************************************************************************\
 ** The particle estimator with a variable kernel width (Grad-H).
\******************************************************************************/
template<class EquationOfState, class Kernel = CubicKernel,
         class ArtificialViscosity = MorrisMonaghanArtificialViscosity>
  requires std::is_object_v<EquationOfState> && std::is_object_v<Kernel> &&
           std::is_object_v<ArtificialViscosity>
class GradHSmoothEstimator {
private:

  EquationOfState _eos;
  Kernel _kernel;
  ArtificialViscosity _artvisc;
  real_t _coupling;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed} | // TODO: fixed should not be here.
      meta::Set{h, Omega, m, rho, p, cs, r, v, dv_dt} |
      EquationOfState::required_fields | Kernel::required_fields |
      ArtificialViscosity::required_fields;

  /** Initialize particle estimator. */
  constexpr GradHSmoothEstimator( //
      EquationOfState eos = {}, Kernel kernel = {},
      ArtificialViscosity viscosity = {}, real_t coupling = 1.0) noexcept
      : _kernel{std::move(kernel)}, _eos{std::move(eos)},
        _artvisc{std::move(viscosity)}, _coupling{coupling} {}

  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void init(ParticleArray& particles) const {
    const auto eta = _coupling;
    particles.for_each([&](auto a) {
      if (!fixed[a]) return;
      const auto d = dim(r[a]);
      // Init particle width (and Omega).
      h[a] = eta * pow(rho[a] / m[a], -inverse(1.0 * d));
      Omega[a] = 1.0;
      // Init particle pressure (and sound speed).
      _eos.compute_pressure(a);
    });
  }

  /** Estimate density, kernel width, pressure and sound speed. */
  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void estimate_density(ParticleArray& particles) const {
    const auto eta = _coupling;
    particles.for_each([&](auto a) {
      if (fixed[a]) return;
      const auto d = dim(r[a]);
      // Compute particle density, width (and Omega).
      // Solve zeta(h) = 0 for h, where: zeta(h) = Rho(h) - rho(h),
      // Rho(h) = m * (eta / h)^d - desired density.
      newton_raphson(h[a], [&] {
        rho[a] = {};
        Omega[a] = {};
        const auto search_radius = _kernel.radius(h[a]);
        particles.nearby(a, search_radius, [&]<class B>(B b) {
          rho[a] += m[b] * _kernel(r[a, b], h[a]);
          Omega[a] += m[b] * _kernel.radius_deriv(r[a, b], h[a]);
        });
        const auto Rho_a = m[a] * pow(eta / h[a], d);
        const auto dRho_dh_a = -d * Rho_a / h[a];
        const auto zeta_a = Rho_a - rho[a];
        const auto dzeta_dh_a = dRho_dh_a - Omega[a];
        Omega[a] = 1.0 - Omega[a] / dRho_dh_a;
        return std::tuple{zeta_a, dzeta_dh_a};
      });
      // Compute particle pressure (and sound speed).
      _eos.compute_pressure(a);
    });
    // Compute velocity divergence and curl.
    particles.for_each([&]<class PV>(PV a) {
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
      const auto search_radius = _kernel.radius(h[a]);
      particles.nearby(a, search_radius, [&]<class B>(B b) {
        const auto grad_aba = _kernel.grad(r[a, b], h[a]);
        const auto grad_abb = _kernel.grad(r[a, b], h[b]);
        if constexpr (has<PV>(div_v)) {
          div_v[a] += m[b] * (dot(v[a] / pow2(rho[a]), grad_aba) +
                              dot(v[b] / pow2(rho[b]), grad_abb));
        }
        if constexpr (has<PV>(curl_v)) {
          curl_v[a] -= m[b] * (cross(v[a] / pow2(rho[a]), grad_aba) +
                               cross(v[b] / pow2(rho[b]), grad_abb));
        }
      });
      if constexpr (has<PV>(div_v)) div_v[a] *= rho[a];
      if constexpr (has<PV>(curl_v)) curl_v[a] *= rho[a];
    });
  }

  /** Estimate acceleration and thermal heating. */
  template<class ParticleArray>
    requires (has<ParticleArray>(required_fields))
  constexpr void estimate_forces(ParticleArray& particles) const {
    particles.for_each([&]<class PV>(PV a) {
      if (fixed[a]) return;
      // Compute velocity and thermal energy forces.
      dv_dt[a] = {};
      if constexpr (has<PV>(eps, deps_dt)) {
        deps_dt[a] = {};
      }
      const auto d = dim(r[a]);
      const auto search_radius = _kernel.radius(h[a]);
      particles.nearby(a, search_radius, [&](PV b) {
        const auto Pi_ab = _artvisc.kinematic(a, b);
        const auto grad_aba = _kernel.grad(r[a, b], h[a]);
        const auto grad_abb = _kernel.grad(r[a, b], h[b]);
        const auto grad_ab = avg(grad_aba, grad_abb);
        dv_dt[a] -= m[b] * (p[a] / (Omega[a] * pow2(rho[a])) * grad_aba +
                            p[b] / (Omega[b] * pow2(rho[b])) * grad_abb +
                            Pi_ab * grad_ab);
        if constexpr (has<PV>(eps, deps_dt)) {
          // clang-format off
          deps_dt[a] += m[b] * (p[a] / (Omega[a] * pow2(rho[a])) *
                                                        dot(grad_aba, v[a, b]) +
                                Pi_ab * dot(grad_ab, v[a, b]));
          // clang-format on
        }
      });
#if 1
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
      // Compute artificial viscosity switch forces.
      if constexpr (has<PV>(dalpha_dt)) {
        _artvisc.compute_switch_deriv(a);
      }
    });
  }

}; // class GradHSmoothEstimator
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
