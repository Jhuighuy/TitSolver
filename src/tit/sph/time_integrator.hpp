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
#include <type_traits> // IWYU pragma: keep

#include "tit/core/meta.hpp"
#include "tit/core/types.hpp"

#include "TitParticle.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/smooth_estimator.hpp" // IWYU pragma: keep

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
  size_t _step_index;
  size_t _adjacency_recalc_freq;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed, r, v, dv_dt} | SmoothEstimator::required_fields;

  /** Construct time integrator. */
  constexpr EulerIntegrator(SmoothEstimator estimator = {},
                            size_t adjacency_recalc_freq = 10) noexcept
      : _estimator{std::move(estimator)}, _step_index{0},
        _adjacency_recalc_freq{adjacency_recalc_freq} {}

  /** Make a step in time. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void step(real_t dt, ParticleArray& particles,
                      ParticleAdjacency& adjacent_particles) {
    using PV = ParticleView<ParticleArray>;
    // Initialize and index particles.
    if (_step_index == 0) {
      // Initialize particles.
      _estimator.init(particles);
    }
    if (_step_index % _adjacency_recalc_freq == 0) {
      // Update particle adjacency.
      _estimator.index(particles, adjacent_particles);
    }
    // Integrate particle density.
    _estimator.compute_density(particles, adjacent_particles);
    if constexpr (has<PV>(drho_dt)) {
      std::ranges::for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        rho[a] += dt * drho_dt[a];
      });
    }
    // Integrate particle velocty (internal enegry, and rest).
    _estimator.compute_forces(particles, adjacent_particles);
    std::ranges::for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
      // Velocity is updated first, so the integrator is semi-implicit.
      v[a] += dt * dv_dt[a];
      if constexpr (has<PV>(v_xsph)) {
        // TODO: extract "0.5" to parameter epsilon.
        r[a] += dt * (v[a] - 0.5 * v_xsph[a]);
      } else {
        r[a] += dt * v[a];
      }
      if constexpr (has<PV>(eps, deps_dt)) eps[a] += dt * deps_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });
    ++_step_index;
  }

}; // class EulerIntegrator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
