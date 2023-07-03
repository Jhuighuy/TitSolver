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

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<class Real, dim_t Dim>
class SmoothEstimator {
public:

  virtual ~SmoothEstimator() = default;

  /** Estimate density, kernel width, pressure and sound speed. */
  constexpr virtual void estimate_density( //
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const EquationOfState<Real>& eos) const = 0;

  /** Estimate acceleration and thermal heating. */
  constexpr virtual void estimate_acceleration(
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const ArtificialViscosity<Real, Dim>& viscosity) const = 0;

}; // class SmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a fixed kernel width.
\******************************************************************************/
template<class Real, dim_t Dim>
class ClassicSmoothEstimator : public SmoothEstimator<Real, Dim> {
private:

  Real _kernel_width;

public:

  constexpr explicit ClassicSmoothEstimator(Real kernel_width) noexcept
      : _kernel_width{kernel_width} {}

  /** Estimate density, kernel width, pressure and sound speed. */
  constexpr void estimate_density( //
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const EquationOfState<Real>& eos) const override {
    using namespace particle_accessors;
    const auto h = _kernel_width;
    const auto search_radius = kernel.radius(h);
    particles.for_each([&](Particle<Real, Dim>& a) {
      h[a] = h;
      rho[a] = {};
      particles.nearby(a, search_radius, [&](const Particle<Real, Dim>& b) {
        rho[a] += m[b] * kernel(r[a, b], h);
      });
      p[a] = eos.pressure(rho[a], eps[a]);
      cs[a] = eos.sound_speed(rho[a], p[a]);
    });
  }

  /** Estimate acceleration and thermal heating. */
  constexpr void estimate_acceleration(
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const ArtificialViscosity<Real, Dim>& viscosity) const override {
    using namespace particle_accessors;
    const auto h = _kernel_width;
    const auto search_radius = kernel.radius(h);
    particles.for_each([&](Particle<Real, Dim>& a) {
      dv_dt[a] = {};
      deps_dt[a] = {};
      particles.nearby(a, search_radius, [&](const Particle<Real, Dim>& b) {
        const auto Pi_ab = viscosity.kinematic(a, b);
        const auto grad_ab = kernel.grad(r[a, b], h);
        // clang-format off
        dv_dt[a] -= m[b] * (p[a] / pow2(rho[a]) + 
                            p[b] / pow2(rho[b]) + 
                            Pi_ab) * grad_ab;
        deps_dt[a] += m[b] * (p[a] / pow2(rho[a]) + 
                              Pi_ab) * dot(grad_ab, v(a, b));
        // clang-format on
      });
    });
  }

}; // class ClassicSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The particle estimator with a variable kernel width (Grad-H).
\******************************************************************************/
template<class Real, dim_t Dim>
class GradHSmoothEstimator : public SmoothEstimator<Real, Dim> {
private:

  Real _coupling;

public:

  constexpr explicit GradHSmoothEstimator(Real coupling = Real{3.0}) noexcept
      : _coupling{coupling} {}

  /** Estimate density, kernel width, pressure and sound speed. */
  constexpr void estimate_density( //
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const EquationOfState<Real>& eos) const override {
    using namespace particle_accessors;
    particles.for_each([&](Particle<Real, Dim>& a) {
      // Solve rho(h) * h^Dim = coupling for h.
      newton(h[a], [&]() {
        rho[a] = {};
        drho_dh[a] = {};
        const auto search_radius = kernel.radius(h[a]);
        particles.nearby(a, search_radius, [&](const Particle<Real, Dim>& b) {
          rho[a] += m[b] * kernel(r[a, b], h[a]);
          drho_dh[a] += m[b] * kernel.radius_deriv(r[a, b], h[a]);
        });
        const auto rho_expected = m[a] * pow(divide(_coupling, h[a]), Dim);
        const auto drho_dh_expected = -Dim * rho_expected / h[a];
        return std::pair{rho_expected - rho[a], drho_dh_expected - drho_dh[a]};
      });
      p[a] = eos.pressure(rho[a], eps[a]);
      cs[a] = eos.sound_speed(rho[a], p[a]);
    });
  }

  /** Estimate acceleration and thermal heating. */
  constexpr void estimate_acceleration(
      ParticleArray<Real, Dim>& particles,
      const SmoothingKernel<Real, Dim>& kernel,
      const ArtificialViscosity<Real, Dim>& viscosity) const final {
    using namespace particle_accessors;
    particles.for_each([&](Particle<Real, Dim>& a) {
      dv_dt[a] = {};
      deps_dt[a] = {};
      const auto Omega_a = Real{1.0} + h[a] / (Dim * rho[a]) * drho_dh[a];
      const auto search_radius = kernel.radius(h[a]);
      particles.nearby(a, search_radius, [&](const Particle<Real, Dim>& b) {
        const auto Omega_b = Real{1.0} + h[b] / (Dim * rho[b]) * drho_dh[b];
        const auto Pi_ab = viscosity.kinematic(a, b);
        const auto grad_aba = kernel.grad(r[a, b], h[a]);
        const auto grad_abb = kernel.grad(r[a, b], h[b]);
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
