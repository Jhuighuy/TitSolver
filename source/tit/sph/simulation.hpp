/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <utility>

#include "tit/core/profiler.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Distributed simulation phase orchestrator.
template<class Equations, class Integrator>
class Simulation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | Integrator::required_fields;

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | Integrator::modified_fields;

  /// Construct a simulation from numerical policies and a communicator.
  explicit Simulation(Equations equations,
                      Integrator integrator,
                      dist::Communicator communicator)
      : equations_{std::move(equations)}, integrator_{std::move(integrator)},
        communicator_{std::move(communicator)} {}

  /// Initialize equation-owned fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void initialize(ParticleMesh& mesh, ParticleArray& particles) const {
    equations_.initialize(mesh, particles);
  }

  /// Advance one globally synchronized explicit step.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto step(ParticleMesh& mesh, ParticleArray& particles) const
      -> particle_num_t<ParticleArray> {
    TIT_PROFILE_SECTION("Simulation::step()");
    using Num = particle_num_t<ParticleArray>;

    const auto initial_state = integrator_.capture(particles);
    Num dt{};
    for (std::size_t stage = 0; stage < integrator_.num_stages(); ++stage) {
      // State halo exchange will precede this local preparation phase once a
      // distributed particle topology is attached to the simulation.
      equations_.prepare(mesh, particles);

      if (stage == 0) {
        const auto local_dt = equations_.compute_time_step(particles);
        dt = communicator_.all_reduce_min(local_dt);
      }

      equations_.compute_continuity(mesh, particles);
      equations_.compute_momentum(mesh, particles);
      integrator_.advance_stage(stage, initial_state, particles, dt);
    }

    // Refresh accepted state before post-integration corrections.
    equations_.prepare(mesh, particles);
    equations_.compute_shift_fields(mesh, particles);

    // Exchange `N` before local surface classification when halos are active.
    equations_.classify_free_surface(mesh, particles);

    // Exchange `phi` before reading neighboring surface classifications.
    equations_.classify_near_surface(mesh, particles);
    equations_.apply_particle_shifts(particles);

    // Exchange shifted position, velocity, and density before correction.
    equations_.prepare_density_correction(particles);
    equations_.apply_density_correction(mesh, particles);
    return dt;
  }

  /// Communicator used by this simulation.
  auto communicator() const noexcept -> const dist::Communicator& {
    return communicator_;
  }

private:

  [[no_unique_address]] Equations equations_;
  [[no_unique_address]] Integrator integrator_;
  dist::Communicator communicator_;

}; // class Simulation

template<class Equations, class Integrator>
Simulation(Equations, Integrator, dist::Communicator)
    -> Simulation<Equations, Integrator>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
