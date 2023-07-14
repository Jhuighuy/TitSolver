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
#include "tit/sph/smooth_estimator.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Semi-implicit Euler time integrator.
\******************************************************************************/
template<class SmoothEstimator>
  requires std::is_object_v<SmoothEstimator>
class EulerIntegrator {
private:

  SmoothEstimator _estimator{};
  mutable size_t _count;

public:

  /** Construct time integrator. */
  constexpr EulerIntegrator(SmoothEstimator estimator = {})
      : _estimator{std::move(estimator)}, _count{0} {}

  /** Make a step in time. */
  template<class Real, class ParticleCloud>
  constexpr void step(Real dt, ParticleCloud& particles) const {
    if (_count++ == 0) _estimator.init(particles);
    particles.sort();
    _estimator.estimate_density(particles);
    _estimator.estimate_forces(particles);
    particles.for_each([&]<class A>(A a) {
      using namespace particle_fields;
      if (fixed[a]) return;
      // Velocity is updated first, so the integrator is semi-implicit.
      v[a] += dt * dv_dt[a];
      r[a] += dt * dr_dt[a];
      if constexpr (has<A>(eps, deps_dt)) eps[a] += dt * deps_dt[a];
      if constexpr (has<A>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });
  }

}; // class EulerIntegrator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
