/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/profiler.hpp"

#include "tit/core/type_utils.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Explicit equations type.
template<class EE>
concept explicit_equations = specialization_of<EE, FluidEquations>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Kick-Drift Euler time integrator.
template<explicit_equations Equations>
class KickDriftIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | meta::Set{parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | meta::Set{parinfo, r, v, u, alpha};

  /// Construct time integrator.
  ///
  /// @param equations Equations to integrate.
  constexpr explicit KickDriftIntegrator(Equations equations,
                                         size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void step(particle_num_t<ParticleArray> dt,
            ParticleMesh& mesh,
            ParticleArray& particles) {
    TIT_PROFILE_SECTION("EulerIntegrator::step()");
    using PV = ParticleView<ParticleArray>;

    // Initialize particles, build the mesh.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Setup boundary conditions.
    equations_.setup_boundary(mesh, particles);

    // Update particle density.
    equations_.compute_density(mesh, particles);
    if constexpr (has<PV>(drho_dt)) {
      par::for_each(par::static_, particles.fluid(), [dt](PV a) {
        rho[a] += dt * drho_dt[a];
      });
    }

    // Update particle velocty, internal energy, etc.
    equations_.compute_forces(mesh, particles);
    par::for_each(par::static_, particles.fluid(), [dt](PV a) {
      v[a] += dt * dv_dt[a];
      r[a] += dt * v[a]; // Kick-Drift: position is updated after velocity.
      if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });

    // Apply particle shifting.
    if constexpr (has<PV>(dr)) {
      equations_.compute_shifts(mesh, particles);
      par::for_each(par::static_, particles.fluid(), [](PV a) {
        r[a] += dr[a];
      });
    }

    // Increment step index.
    step_index_ += 1;
  }

private:

  [[no_unique_address]] Equations equations_{};
  size_t mesh_update_freq_;
  size_t step_index_ = 0;

}; // class KickDriftIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Kick-Drift-Kick Leapfrog time integrator.
template<explicit_equations Equations>
class KickDriftKickIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | meta::Set{parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | meta::Set{parinfo, r, v, u, alpha};

  /// Construct time integrator.
  ///
  /// @param equations Equations to integrate.
  constexpr explicit KickDriftKickIntegrator(
      Equations equations,
      size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void step(particle_num_t<ParticleArray> dt,
            ParticleMesh& mesh,
            ParticleArray& particles) {
    TIT_PROFILE_SECTION("LeapfrogIntegrator::step()");
    using PV = ParticleView<ParticleArray>;

    // Initialize and index particles.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Setup boundary conditions.
    equations_.setup_boundary(mesh, particles);

    // Update particle velocity to the half step, and position to the full
    // step.
    const auto dt_2 = dt / 2;
    equations_.compute_forces(mesh, particles);
    par::for_each(par::static_, particles.fluid(), [dt, dt_2](PV a) {
      v[a] += dt_2 * dv_dt[a];
      r[a] += dt * v[a]; // Kick-Drift: position is updated after velocity.
      if constexpr (has<PV>(u, du_dt)) u[a] += dt_2 * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt_2 * dalpha_dt[a];
    });

    // Update particle velocity to the full step.
    equations_.compute_density(mesh, particles);
    if constexpr (has<PV>(drho_dt)) {
      par::for_each(par::static_, particles.fluid(), [dt](PV a) {
        rho[a] += dt * drho_dt[a];
      });
    }

    // Update particle velocity to the full step.
    equations_.compute_forces(mesh, particles);
    par::for_each(par::static_, particles.fluid(), [dt_2](PV a) {
      v[a] += dt_2 * dv_dt[a]; // Kick.
      if constexpr (has<PV>(u, du_dt)) u[a] += dt_2 * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt_2 * dalpha_dt[a];
    });

    // Apply particle shifting.
    if constexpr (has<PV>(dr)) {
      equations_.compute_shifts(mesh, particles);
      par::for_each(par::static_, particles.fluid(), [](PV a) {
        r[a] += dr[a];
      });
    }

    // Increment step index.
    step_index_ += 1;
  }

private:

  [[no_unique_address]] Equations equations_{};
  size_t mesh_update_freq_;
  size_t step_index_ = 0;

}; // class KickDriftKickIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Runge-Kutta time integrator (SSPRK(3,3)).
template<explicit_equations Equations>
class RungeKuttaIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | meta::Set{parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | meta::Set{parinfo, r, v, u, alpha};

  /// Construct time integrator.
  constexpr explicit RungeKuttaIntegrator(Equations equations,
                                          size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void step(particle_num_t<ParticleArray> dt,
            ParticleMesh& mesh,
            ParticleArray& particles) {
    TIT_PROFILE_SECTION("RungeKuttaIntegrator::step()");
    using PV = ParticleView<ParticleArray>;

    // Initialize and index particles.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Run the SSPRK(3,3) substeps.
    /// @todo We should copy only the needed fields, and not the whole array.
    const auto old_particles(particles);
    substep_(dt, mesh, particles);
    substep_(dt, mesh, particles);
    lincomb_(0.75, old_particles, 0.25, particles);
    substep_(dt, mesh, particles);
    lincomb_(1.0 / 3.0, old_particles, 2.0 / 3.0, particles);

    // Apply particle shifting.
    if constexpr (has<PV>(dr)) {
      equations_.compute_shifts(mesh, particles);
      par::for_each(par::static_, particles.fluid(), [](PV a) {
        r[a] += dr[a];
      });
    }

    // Increment step index.
    step_index_ += 1;
  }

private:

  // Do an explicit Euler substep.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void substep_(particle_num_t<ParticleArray> dt,
                ParticleMesh& mesh,
                ParticleArray& particles) {
    using PV = ParticleView<ParticleArray>;

    // Calculate right hand sides for the given particle array.
    equations_.setup_boundary(mesh, particles);
    equations_.compute_density(mesh, particles);
    equations_.compute_forces(mesh, particles);

    // Integrate.
    par::for_each(par::static_, particles.fluid(), [dt](PV a) {
      r[a] += dt * v[a]; // Drift-Kick: position is updated before velocity.
      v[a] += dt * dv_dt[a];
      if constexpr (has<PV>(drho_dt)) rho[a] += dt * drho_dt[a];
      if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });
  }

  // Compute the linear combination of the two substeps.
  template<particle_array<required_fields> ParticleArray>
  static void lincomb_(particle_num_t<ParticleArray> weight,
                       const ParticleArray& particles,
                       particle_num_t<ParticleArray> out_weight,
                       ParticleArray& out_particles) {
    using PV = ParticleView<ParticleArray>;
    par::for_each( //
        par::static_,
        out_particles.fluid(),
        [out_weight, weight, &particles](PV out_a) {
          const auto a = particles[out_a.index()];
          r[out_a] = weight * r[a] + out_weight * r[out_a];
          v[out_a] = weight * v[a] + out_weight * v[out_a];
          rho[out_a] = weight * rho[a] + out_weight * rho[out_a];
          if constexpr (has<PV>(u)) {
            u[out_a] = weight * u[a] + out_weight * u[out_a];
          }
          if constexpr (has<PV>(alpha)) {
            alpha[out_a] = weight * alpha[a] + out_weight * alpha[out_a];
          }
        });
  }

  [[no_unique_address]] Equations equations_;
  size_t mesh_update_freq_;
  size_t step_index_ = 0;

}; // class RungeKuttaIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
