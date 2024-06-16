/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
// #include "tit/core/type_traits.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
// #include "tit/sph/fluid_equations.hpp"

namespace tit::sph {
/// Explicit equations type.
template<class EE>
concept explicit_equations = true; // specialization_of<EE, FluidEquations>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Drift-Kick Euler time integrator.
template<explicit_equations Equations>
class EulerIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | //
      meta::Set{fixed, parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | //
      meta::Set{fixed, parinfo, r, v, u, alpha};

  /// Construct time integrator.
  ///
  /// @param equations Equations to integrate.
  constexpr explicit EulerIntegrator(Equations equations,
                                     size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void step(real_t dt, ParticleMesh& mesh, ParticleArray& particles) {
    using PV = ParticleView<ParticleArray>;
    TIT_PROFILE_SECTION("EulerIntegrator::step()");

    // Initialize particles, build the mesh.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Setup boundary conditions.
    equations_.setup_boundary(mesh, particles);

    // Update particle velocty, internal energy, etc.
    equations_.compute_forces(mesh, particles);
    par::for_each(particles.all(), [dt](PV a) {
      if (fixed[a]) return;
      v[a] += dt * dv_dt[a];
      r[a] += dt * v[a]; // important: position is updated after velocity!
      if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
    });

    // Update particle density.
    equations_.compute_density(mesh, particles);
    if constexpr (has<PV>(drho_dt)) {
      par::for_each(particles.all(), [dt](PV a) {
        if (fixed[a]) return;
        rho[a] += dt * drho_dt[a];
      });
    }

    // Increment step index.
    step_index_ += 1;
  }

private:

  [[no_unique_address]] Equations equations_{};
  size_t step_index_{0};
  size_t mesh_update_freq_;

}; // class EulerIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Drift-Kick-Drift Leapfrog time integrator.
template<explicit_equations Equations>
class LeapfrogIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | //
      meta::Set{fixed, parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | //
      meta::Set{fixed, parinfo, r, v, u, alpha};

  /// Construct time integrator.
  ///
  /// @param equations Equations to integrate.
  constexpr explicit LeapfrogIntegrator(Equations equations,
                                        size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void step(real_t dt, ParticleMesh& mesh, ParticleArray& particles) {
    using PV = ParticleView<ParticleArray>;
    TIT_PROFILE_SECTION("LeapfrogIntegrator::step()");

    // Initialize and index particles.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Setup boundary conditions.
    equations_.setup_boundary(mesh, particles);

    // Update particle velocity to the half step, and position to the full
    // step.
    const auto dt_2 = dt / 2.0;
    equations_.compute_forces(mesh, particles);
    par::for_each(particles.all(), [dt, dt_2](PV a) {
      if (fixed[a]) return;
      v[a] += dt_2 * dv_dt[a];
      r[a] += dt * v[a]; // important: position is updated after velocity!
      if constexpr (has<PV>(u, du_dt)) u[a] += dt_2 * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt_2 * dalpha_dt[a];
    });

    // Update particle velocity to the full step.
    equations_.compute_density(mesh, particles);
    if constexpr (has<PV>(drho_dt)) {
      par::for_each(particles.all(), [dt](PV a) {
        if (fixed[a]) return;
        rho[a] += dt * drho_dt[a];
      });
    }

    // Update particle velocity to the full step.
    equations_.compute_forces(mesh, particles);
    par::for_each(particles.all(), [dt_2](PV a) {
      if (fixed[a]) return;
      v[a] += dt_2 * dv_dt[a];
      if constexpr (has<PV>(u, du_dt)) u[a] += dt_2 * du_dt[a];
      if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt_2 * dalpha_dt[a];
    });

    // Increment step index.
    step_index_ += 1;
  }

private:

  [[no_unique_address]] Equations equations_{};
  size_t step_index_{0};
  size_t mesh_update_freq_;

}; // class LeapfrogIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Runge-Kutta time integrator (SSPRK(3,3)).
template<explicit_equations Equations>
class RungeKuttaIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | //
      meta::Set{fixed, parinfo, r, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | //
      meta::Set{fixed, parinfo, r, v, u, alpha};

  /// Construct time integrator.
  constexpr explicit RungeKuttaIntegrator(Equations equations,
                                          size_t mesh_update_freq = 10) noexcept
      : equations_{std::move(equations)}, mesh_update_freq_{mesh_update_freq} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void step(real_t dt, ParticleMesh& mesh, ParticleArray& particles) {
    using PV = ParticleView<ParticleArray>;
    TIT_PROFILE_SECTION("RungeKuttaIntegrator::step()");

    // Initialize and index particles.
    if (step_index_ == 0) equations_.init(particles);
    if (step_index_ % mesh_update_freq_ == 0) equations_.index(mesh, particles);

    // Do an explicit Euler substep.
    const auto substep = [&](auto& _particles) {
      // Calculate right hand sides for the given particle array.
      equations_.setup_boundary(mesh, _particles);
      equations_.compute_density(mesh, _particles);
      equations_.compute_forces(mesh, _particles);
      // Integrate.
      par::for_each(_particles.all(), [&](PV a) {
        if (fixed[a]) return;
        r[a] += dt * v[a]; // important: position is updated before velocity!
        v[a] += dt * dv_dt[a];
        if constexpr (has<PV>(drho_dt)) rho[a] += dt * drho_dt[a];
        if constexpr (has<PV>(u, du_dt)) u[a] += dt * du_dt[a];
        if constexpr (has<PV>(alpha, dalpha_dt)) alpha[a] += dt * dalpha_dt[a];
      });
    };

    // Linear combination.
    const auto lincomb = [](real_t wa,
                            const auto& in_particles,
                            real_t wb,
                            auto& out_particles) {
      par::for_each(in_particles.all(), [&](auto a) {
        if (fixed[a]) return;
        auto out_a = out_particles[a.index()];
        v[out_a] = wa * v[a] + wb * v[out_a];
        r[out_a] = wa * r[a] + wb * r[out_a];
        rho[out_a] = wa * rho[a] + wb * rho[out_a];
        if constexpr (has<PV>(u)) u[out_a] = wa * u[a] + wb * u[out_a];
        if constexpr (has<PV>(alpha)) {
          alpha[out_a] = wa * alpha[a] + wb * alpha[out_a];
        }
      });
    };

    {
      auto& u = particles;
      auto u1 = auto(u);
      substep(u);
      substep(u);
      lincomb(0.75, u1, 0.25, u);
      substep(u);
      lincomb(1.0 / 3.0, u1, 2.0 / 3.0, u);
    }

    // Increment step index.
    step_index_ += 1;
  }

private:

  [[no_unique_address]] Equations equations_;
  size_t step_index_{0};
  size_t mesh_update_freq_;

}; // class RungeKuttaIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
