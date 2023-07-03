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

#include "TitParticle.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/smooting_kernel.hpp"
#include "tit/utils/math.hpp"
#include "tit/utils/vec.hpp"

#include <optional>
#include <type_traits>

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<class EquationOfState = GasEquationOfState,
         class SmoothingKernel = CubicSmoothingKernel,
         class ArtificialViscosity = AlphaBetaArtificialViscosity>
  requires std::is_object_v<EquationOfState> &&
           std::is_object_v<SmoothingKernel> &&
           std::is_object_v<ArtificialViscosity>
class ClassicSmoothEstimator final {
private:

  EquationOfState _eos;
  SmoothingKernel _kernel;
  ArtificialViscosity _viscosity;
  std::optional<real_t> _kernel_width;

public:

  /** Initialize particle estimator. */
  constexpr explicit ClassicSmoothEstimator(
      EquationOfState eos = {}, SmoothingKernel kernel = {},
      ArtificialViscosity viscosity = {},
      std::optional<real_t> kernel_width = std::nullopt)
      : _eos{std::move(eos)}, _kernel{std::move(kernel)},
        _viscosity{std::move(viscosity)}, _kernel_width{kernel_width} {}

  /** Estimator equation of state. */
  constexpr const EquationOfState& eos() const noexcept {
    return _eos;
  }

  /** Estimator kernel. */
  constexpr const SmoothingKernel& kernel() const noexcept {
    return _kernel;
  }

  /** Estimator artificial viscosity scheme. */
  constexpr const ArtificialViscosity& viscosity() const noexcept {
    return _viscosity;
  }

  /** Estimate density, kernel width, pressure and sound speed. */
  template<class ParticleCloud>
  constexpr void estimate_density(ParticleCloud& particles) const {
    using namespace particle_accessors;
    const auto h_ab = *_kernel_width;
    const auto search_radius = _kernel.radius(h_ab);
    particles.for_each([&](auto a) {
      h[a] = h_ab;
      rho[a] = {};
      particles.nearby(a, search_radius, [&](auto b) { //
        rho[a] += m[b] * _kernel(r[a, b], h_ab);
      });
      p[a] = _eos.pressure(rho[a], eps[a]);
      cs[a] = _eos.sound_speed(rho[a], p[a]);
    });
  }

  /** Estimate acceleration and thermal heating. */
  template<class ParticleCloud>
  constexpr void estimate_acceleration(ParticleCloud& particles) const {
    using namespace particle_accessors;
    const auto h = _kernel_width;
    const auto search_radius = _kernel.radius(h);
    particles.for_each([&](auto a) {
      dv_dt[a] = {};
      deps_dt[a] = {};
      particles.nearby(a, search_radius, [&](auto b) {
        const auto Pi_ab = _viscosity.kinematic(a, b);
        const auto grad_ab = _kernel.grad(r[a, b], h);
        // clang-format off
        dv_dt[a] -= m[b] * (p[a] / pow2(rho[a]) +
                            p[b] / pow2(rho[b]) +
                            Pi_ab) * grad_ab;
        deps_dt[a] += m[b] * (p[a] / pow2(rho[a]) +
                              Pi_ab) * dot(grad_ab, v[a, b]);
        // clang-format on
      });
    });
  }

}; // class ClassicSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a variable kernel width (Grad-H).
\******************************************************************************/
template<class EquationOfState = GasEquationOfState,
         class SmoothingKernel = CubicSmoothingKernel,
         class ArtificialViscosity = AlphaBetaArtificialViscosity>
  requires std::is_object_v<EquationOfState> &&
           std::is_object_v<SmoothingKernel> &&
           std::is_object_v<ArtificialViscosity>
class GradHSmoothEstimator final {
private:

  EquationOfState _eos;
  SmoothingKernel _kernel;
  ArtificialViscosity _viscosity;
  real_t _coupling;

public:

  /** Initialize particle estimator. */
  constexpr explicit GradHSmoothEstimator( //
      EquationOfState eos = {}, SmoothingKernel kernel = {},
      ArtificialViscosity viscosity = {}, real_t coupling = 3.0) noexcept
      : _kernel{std::move(kernel)}, _eos{std::move(eos)},
        _viscosity{std::move(viscosity)}, _coupling{coupling} {}

  /** Estimator equation of state. */
  constexpr const EquationOfState& eos() const noexcept {
    return _eos;
  }

  /** Estimator kernel. */
  constexpr const SmoothingKernel& kernel() const noexcept {
    return _kernel;
  }

  /** Estimator artificial viscosity scheme. */
  constexpr const ArtificialViscosity& viscosity() const noexcept {
    return _viscosity;
  }

  /** Estimate density, kernel width, pressure and sound speed. */
  template<class ParticleCloud>
  constexpr void estimate_density(ParticleCloud& particles) const {
    using namespace particle_accessors;
    particles.for_each([&](auto a) {
      // Solve rho(h) * h^d = coupling for h.
      const auto d = dim(r[a]);
      newton(h[a], [&]() {
        rho[a] = {};
        drho_dh[a] = {};
        const auto search_radius = _kernel.radius(h[a]);
        particles.nearby(a, search_radius, [&](auto b) {
          rho[a] += m[b] * _kernel(r[a, b], h[a]);
          drho_dh[a] += m[b] * _kernel.radius_deriv(r[a, b], h[a]);
        });
        const auto rho_expected = m[a] * pow(divide(_coupling, h[a]), d);
        const auto drho_dh_expected = -d * rho_expected / h[a];
        return std::pair{rho_expected - rho[a], drho_dh_expected - drho_dh[a]};
      });
      p[a] = _eos.pressure(rho[a], eps[a]);
      cs[a] = _eos.sound_speed(rho[a], p[a]);
    });
  }

  /** Estimate acceleration and thermal heating. */
  template<class ParticleCloud>
  constexpr void estimate_acceleration(ParticleCloud& particles) const {
    using namespace particle_accessors;
    particles.for_each([&](auto a) {
      dv_dt[a] = {};
      deps_dt[a] = {};
      const auto d = dim(r[a]);
      const auto Omega_a = 1 + h[a] / (d * rho[a]) * drho_dh[a];
      const auto search_radius = _kernel.radius(h[a]);
      particles.nearby(a, search_radius, [&](auto b) {
        const auto Omega_b = 1 + h[b] / (d * rho[b]) * drho_dh[b];
        const auto Pi_ab = _viscosity.kinematic(a, b);
        const auto grad_aba = _kernel.grad(r[a, b], h[a]);
        const auto grad_abb = _kernel.grad(r[a, b], h[b]);
        const auto grad_ab = avg(grad_aba, grad_abb);
        dv_dt[a] -= m[b] * (p[a] / (Omega_a * pow2(rho[a])) * grad_aba +
                            p[b] / (Omega_b * pow2(rho[b])) * grad_abb +
                            Pi_ab * grad_ab);
        // clang-format off
        deps_dt[a] += m[b] * (p[a] / (Omega_a * pow2(rho[a])) * dot(grad_aba, v[a, b]) +
                              Pi_ab * dot(grad_ab, v[a, b]));
        // clang-format on
      });
    });
  }

}; // class GradHSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
