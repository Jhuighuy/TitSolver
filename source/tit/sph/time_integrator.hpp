/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Strong-stability-preserving Runge-Kutta order.
enum class SSPRKOrder : std::uint8_t {
  two,
  three,
};

namespace impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Persistent owned state at the start of an SSPRK step.
template<class Num, class Vec>
class SSPRKState final {
public:

  /// Capture the persistent fields of all owned particles.
  template<particle_array<r, v, rho> ParticleArray>
  explicit SSPRKState(const ParticleArray& particles) {
    const auto size = particles.num_owned();
    ids_.reserve(size);
    positions_.reserve(size);
    velocities_.reserve(size);
    densities_.reserve(size);
    for (const auto a : particles.owned()) {
      ids_.push_back(a.id());
      positions_.push_back(r[a]);
      velocities_.push_back(v[a]);
      densities_.push_back(rho[a]);
    }
  }

  /// Combine an updated stage with the state at the start of the step.
  template<particle_array<r, v, rho> ParticleArray>
  void combine(ParticleArray& particles, Num stage_weight) const {
    TIT_ASSERT(particles.num_owned() == ids_.size(),
               "Owned particle count changed during an SSPRK step.");
    using PV = ParticleView<ParticleArray>;
    par::for_each(particles.owned(), [this, stage_weight](PV a) {
      const auto index = a.index();
      TIT_ASSERT(a.id() == ids_[index],
                 "Owned particle ordering changed during an SSPRK step.");
      const auto initial_weight = Num{1} - stage_weight;
      r[a] = initial_weight * positions_[index] + stage_weight * r[a];
      v[a] = initial_weight * velocities_[index] + stage_weight * v[a];
      rho[a] = initial_weight * densities_[index] + stage_weight * rho[a];
    });
  }

private:

  std::vector<ParticleID> ids_;
  std::vector<Vec> positions_;
  std::vector<Vec> velocities_;
  std::vector<Num> densities_;

}; // class SSPRKState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl

/// Strong-stability-preserving Runge-Kutta stage integrator.
///
/// This type owns only integration policy and persistent stage state. Equation
/// evaluation, mesh refreshes, communication, and collectives are orchestrated
/// by `Simulation`.
class SSPRKIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = TypeSet{h, r, v, rho, dv_dt, drho_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields = TypeSet{r, v, rho};

  /// Construct the stage integrator.
  constexpr explicit SSPRKIntegrator(
      SSPRKOrder order = SSPRKOrder::three) noexcept
      : order_{order} {}

  /// Number of equation-evaluation stages in one step.
  constexpr auto num_stages() const noexcept -> std::size_t {
    switch (order_) {
      case SSPRKOrder::two:   return 2;
      case SSPRKOrder::three: return 3;
      default:                std::unreachable();
    }
  }

  /// Capture owned state at the beginning of a step.
  template<particle_array<required_fields> ParticleArray>
  auto capture(const ParticleArray& particles) const {
    return impl::SSPRKState<particle_num_t<ParticleArray>,
                            particle_vec_t<ParticleArray>>{particles};
  }

  /// Apply an Euler update and the SSPRK combination for a stage.
  template<particle_array<required_fields> ParticleArray>
  void advance_stage(
      std::size_t stage,
      const impl::SSPRKState<particle_num_t<ParticleArray>,
                             particle_vec_t<ParticleArray>>& initial_state,
      ParticleArray& particles,
      particle_num_t<ParticleArray> dt) const {
    TIT_PROFILE_SECTION("SSPRKIntegrator::advance_stage()");
    TIT_ASSERT(stage < num_stages(), "Invalid SSPRK stage index.");
    using Num = particle_num_t<ParticleArray>;
    using PV = ParticleView<ParticleArray>;

    par::for_each(particles.owned(), [dt](PV a) {
      r[a] += dt * v[a];
      v[a] += dt * dv_dt[a];
      rho[a] += dt * drho_dt[a];
    });

    if (stage == 0) return;
    switch (order_) {
      case SSPRKOrder::two:
        initial_state.combine(particles, Num{1.0 / 2.0});
        break;
      case SSPRKOrder::three:
        initial_state.combine(particles,
                              stage == 1 ? Num{1.0 / 4.0} : Num{2.0 / 3.0});
        break;
      default: std::unreachable();
    }
  }

private:

  SSPRKOrder order_;

}; // class SSPRKIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
