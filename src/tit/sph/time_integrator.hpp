/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits> // IWYU pragma: keep

#include "tit/core/meta.hpp"
#include "tit/core/types.hpp"
#include "tit/par/thread.hpp"

#include "TitParticle.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp" // IWYU pragma: keep

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Semi-implicit Euler time integrator.
\******************************************************************************/
template<class FluidEquations>
  requires std::is_object_v<FluidEquations>
class EulerIntegrator {
private:

  FluidEquations equations_{};
  size_t step_index_;
  size_t adjacency_recalc_freq_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed, r, v, dv_dt} | FluidEquations::required_fields;

  /** Construct time integrator. */
  constexpr EulerIntegrator(FluidEquations estimator = {},
                            size_t adjacency_recalc_freq = 10) noexcept
      : equations_{std::move(estimator)}, step_index_{0},
        adjacency_recalc_freq_{adjacency_recalc_freq} {}

  /** Make a step in time. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void step(real_t dt, ParticleArray& particles,
                      ParticleAdjacency& adjacent_particles) {
    using PV = ParticleView<ParticleArray>;
    // Initialize and index particles.
    if (step_index_ == 0) {
      // Initialize particles.
      equations_.init(particles);
    }
    if (step_index_ % adjacency_recalc_freq_ == 0) {
      // Update particle adjacency.
      equations_.index(particles, adjacent_particles);
    }
    // Integrate particle density.
    equations_.compute_density(particles, adjacent_particles);
    if constexpr (has<PV>(drho_dt)) {
      par::static_for_each(particles.views(), [&](PV a) {
        if (fixed[a]) return;
        rho[a] += dt * drho_dt[a];
      });
    }
    // Integrate particle velocty (internal enegry, and rest).
    equations_.compute_forces(particles, adjacent_particles);
    par::static_for_each(particles.views(), [&](PV a) {
      if (fixed[a]) return;
      // Velocity is updated first, so the integrator is semi-implicit.
      v[a] += dt * dv_dt[a];
      if constexpr (has<PV>(v_xsph)) {
        // TODO: extract "0.5" to parameter epsilon.
        r[a] += dt * (v[a] - 0.5 * v_xsph[a]);
      } else {
        r[a] += dt * v[a];
      }
      if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });
    ++step_index_;
  }

}; // class EulerIntegrator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Runge-Kutta time integrator.
\******************************************************************************/
template<class FluidEquations>
  requires std::is_object_v<FluidEquations>
class RungeKuttaIntegrator {
private:

  FluidEquations equations_{};
  size_t step_index_;
  size_t adjacency_recalc_freq_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{fixed, r, v, dv_dt} | FluidEquations::required_fields;

  /** Construct time integrator. */
  constexpr RungeKuttaIntegrator(FluidEquations estimator = {},
                                 size_t adjacency_recalc_freq = 10) noexcept
      : equations_{std::move(estimator)}, step_index_{0},
        adjacency_recalc_freq_{adjacency_recalc_freq} {}

  /** Make a step in time. */
  template<class ParticleArray, class ParticleAdjacency>
    requires (has<ParticleArray>(required_fields))
  constexpr void step(real_t dt, ParticleArray& particles,
                      ParticleAdjacency& adjacent_particles) {
    // Initialize and index particles.
    if (step_index_ == 0) {
      // Initialize particles.
      equations_.init(particles);
    }
    if (step_index_ % adjacency_recalc_freq_ == 0) {
      // Update particle adjacency.
      equations_.index(particles, adjacent_particles);
    }
#if 1
    // Do an explicit Euler substep.
    const auto substep = [&](auto& particles) {
      // Calculate right hand sides for the given particle array.
      equations_.compute_density(particles, adjacent_particles);
      equations_.compute_forces(particles, adjacent_particles);
      // Integrate.
      par::for_each(particles.views(), [&]<class PV>(PV a) {
        if (fixed[a]) return;
        if constexpr (has<PV>(drho_dt)) rho[a] += dt * drho_dt[a];
        if constexpr (has<PV>(v_xsph)) {
          // TODO: extract "0.5" to parameter epsilon.
          r[a] += dt * (v[a] - 0.5 * v_xsph[a]);
        } else {
          r[a] += dt * v[a];
        }
        v[a] += dt * dv_dt[a];
        if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
        if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
      });
    };
    // Linear combination.
    const auto lincomb = [](real_t wa, const auto& in_particles, //
                            real_t wb, auto& out_particles) {
      par::for_each(in_particles.views(), [&]<class PV>(PV a) {
        if (fixed[a]) return;
        auto out_a = out_particles[a.index()];
        if constexpr (has<PV>(rho) && !has_const<PV>(rho)) {
          rho[out_a] = wa * rho[a] + wb * rho[out_a];
        }
        v[out_a] = wa * v[a] + wb * v[out_a];
        r[out_a] = wa * r[a] + wb * r[out_a];
      });
    };
#if 0
    {
      auto& u = particles;
      substep(u);
    }
#elif 0
    {
      auto& u = particles;
      auto u1 = auto(u);
      substep(u);
      substep(u);
      lincomb(0.5, u1, 0.5, u);
    }
#elif 1
    {
      auto& u = particles;
      auto u1 = auto(u);
      substep(u);
      substep(u);
      lincomb(0.75, u1, 0.25, u);
      substep(u);
      lincomb(1.0 / 3.0, u1, 2.0 / 3.0, u);
    }
#endif
#endif
    ++step_index_;
  }

}; // class RungeKuttaIntegrator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
