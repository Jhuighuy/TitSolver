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

#include "tit/utils/math.hpp"
#include "tit/utils/meta.hpp"
#include "tit/utils/types.hpp"
#include "tit/utils/vec.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/equation_of_state.hpp" // IWYU pragma: keep
#include "tit/sph/field.hpp"
#include "tit/sph/smooth_kernel.hpp"

#include <tuple>
#include <type_traits> // IWYU pragma: keep

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<class EquationOfState, class Kernel = CubicKernel,
         class ArtificialViscosity = MorrisMonaghanArtificialViscosity<>>
  requires std::is_object_v<EquationOfState> && std::is_object_v<Kernel> &&
           std::is_object_v<ArtificialViscosity>
class ClassicSmoothEstimator {
private:

  EquationOfState _eos;
  Kernel _kernel;
  ArtificialViscosity _viscosity;

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
      ArtificialViscosity viscosity = {})
      : _eos{std::move(eos)}, _kernel{std::move(kernel)},
        _viscosity{std::move(viscosity)} {}

  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void init(Particles& particles) const {
    particles.for_each([&](auto a) {
      if (!fixed[a]) return;
      // Init particle pressure (and sound speed).
      _eos.compute_pressure(a);
    });
  }

  /** Estimate density, kernel width, pressure and sound speed. */
  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void estimate_density(Particles& particles) const {
    particles.for_each([&]<class PV>(PV a) {
      if (fixed[a]) return;
      // Compute particle density (or density time derivative).
      const auto search_radius = _kernel.radius(a);
      if constexpr (has<PV>(drho_dt)) {
        // Continuity equation approach.
        drho_dt[a] = {};
        particles.nearby(a, search_radius, [&](PV b) {
          const auto grad_k_ab = _kernel.grad(a, b);
          drho_dt[a] += m[b] * dot(v[a, b], grad_k_ab);
        });
      } else {
        // Density summation approach.
        rho[a] = {};
        particles.nearby(a, search_radius, [&](PV b) {
          const auto k_ab = _kernel(a, b);
          rho[a] += m[b] * k_ab;
        });
      }
      // Compute particle pressure (and sound speed).
      _eos.compute_pressure(a);
    });
    // Compute velocity divergence and curl.
    particles.for_each([&]<class PV>(PV a) {
      if (fixed[a]) return;
      if constexpr (has<PV>(div_v)) div_v[a] = {};
      if constexpr (has<PV>(curl_v)) curl_v[a] = {};
      const auto search_radius = _kernel.radius(a);
      particles.nearby(a, search_radius, [&](PV b) {
        const auto grad_k_ab = _kernel.grad(a, b);
        if constexpr (has<PV>(div_v)) {
          // clang-format off
          div_v[a] += m[b] * dot(v[a] / pow2(rho[a]) +
                                 v[b] / pow2(rho[b]), grad_k_ab);
          // clang-format on
        }
        if constexpr (has<PV>(curl_v)) {
          // clang-format off
          curl_v[a] -= m[b] * cross(v[a] / pow2(rho[a]) +
                                    v[b] / pow2(rho[b]), grad_k_ab);
          // clang-format on
        }
      });
      if constexpr (has<PV>(div_v)) div_v[a] *= rho[a];
      if constexpr (has<PV>(curl_v)) curl_v[a] *= rho[a];
    });
  }

  /** Estimate acceleration and thermal heating. */
  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void estimate_forces(Particles& particles) const {
    particles.for_each([&]<class PV>(PV a) {
      if (fixed[a]) return;
      // Compute velocity and thermal energy forces.
      dv_dt[a] = {};
      if constexpr (has<PV>(eps, deps_dt)) {
        deps_dt[a] = {};
      }
      const auto search_radius = _kernel.radius(a);
      particles.nearby(a, search_radius, [&](PV b) {
        const auto Pi_ab = _viscosity.kinematic(a, b);
        const auto grad_k_ab = _kernel.grad(a, b);
        // clang-format off
        dv_dt[a] -= m[b] * (p[a] / pow2(rho[a]) +
                            p[b] / pow2(rho[b]) + Pi_ab) * grad_k_ab;
        // clang-format on
        if constexpr (has<PV>(eps, deps_dt)) {
          // clang-format off
          deps_dt[a] += m[b] * (p[a] / pow2(rho[a]) +
                                Pi_ab) * dot(grad_k_ab, v[a, b]);
          // clang-format on
        }
      });
#if 1
      // TODO: Gravity.
      dv_dt[a][1] -= 9.81;
#endif
      // Compute artificial viscosity switch forces.
      if constexpr (has<PV>(alpha, dalpha_dt)) {
        _viscosity.compute_switch_deriv(a);
      }
    });
  }

}; // class ClassicSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a variable kernel width (Grad-H).
\******************************************************************************/
template<class EquationOfState, class Kernel = CubicKernel,
         class ArtificialViscosity = MorrisMonaghanArtificialViscosity<>>
  requires std::is_object_v<EquationOfState> && std::is_object_v<Kernel> &&
           std::is_object_v<ArtificialViscosity>
class GradHSmoothEstimator {
private:

  EquationOfState _eos;
  Kernel _kernel;
  ArtificialViscosity _viscosity;
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
        _viscosity{std::move(viscosity)}, _coupling{coupling} {}

  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void init(Particles& particles) const {
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
  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void estimate_density(Particles& particles) const {
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
  template<class Particles>
    requires (has<Particles>(required_fields))
  constexpr void estimate_forces(Particles& particles) const {
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
        const auto Pi_ab = _viscosity.kinematic(a, b);
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
        _viscosity.compute_switch_deriv(a);
      }
    });
  }

}; // class GradHSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
