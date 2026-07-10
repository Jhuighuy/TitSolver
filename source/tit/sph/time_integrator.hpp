/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/par/algorithms.hpp"
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

/// Symplectic Euler time integrator.
template<explicit_equations Equations>
class SymplecticEulerIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | TypeSet{r, dr, v, dv_dt, drho_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | TypeSet{r, v, rho};

  /// Construct time integrator.
  constexpr explicit SymplecticEulerIntegrator(Equations equations) noexcept
      : equations_{std::move(equations)} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto step(ParticleMesh& mesh, ParticleArray& particles) const
      -> particle_num_t<ParticleArray> {
    TIT_PROFILE_SECTION("SymplecticEulerIntegrator::step()");
    using PV = ParticleView<ParticleArray>;

    equations_.prepare(mesh, particles);
    const auto dt = equations_.compute_time_step(particles);

    equations_.compute_continuity(mesh, particles);
    par::for_each(particles.fluid(), [dt](PV a) { rho[a] += dt * drho_dt[a]; });

    equations_.compute_momentum(mesh, particles);
    par::for_each(particles.fluid(), [dt](PV a) {
      v[a] += dt * dv_dt[a];
      r[a] += dt * v[a];
    });

    equations_.post_integrate(mesh, particles);
    return dt;
  }

private:

  [[no_unique_address]] Equations equations_{};

}; // class SymplecticEulerIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Velocity Verlet time integrator.
template<explicit_equations Equations>
class VelocityVerletIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | TypeSet{r, dr, v, dv_dt, drho_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | TypeSet{r, v, rho};

  /// Construct time integrator.
  constexpr explicit VelocityVerletIntegrator(Equations equations) noexcept
      : equations_{std::move(equations)} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto step(ParticleMesh& mesh, ParticleArray& particles) const
      -> particle_num_t<ParticleArray> {
    TIT_PROFILE_SECTION("VelocityVerletIntegrator::step()");
    using PV = ParticleView<ParticleArray>;

    equations_.prepare(mesh, particles);
    const auto dt = equations_.compute_time_step(particles);
    const auto dt_2 = dt / 2;

    equations_.compute_momentum(mesh, particles);
    par::for_each(particles.fluid(), [dt, dt_2](PV a) {
      v[a] += dt_2 * dv_dt[a];
      r[a] += dt * v[a];
    });

    equations_.prepare(mesh, particles);
    equations_.compute_continuity(mesh, particles);
    par::for_each(particles.fluid(), [dt](PV a) { rho[a] += dt * drho_dt[a]; });

    equations_.compute_momentum(mesh, particles);
    par::for_each(particles.fluid(), [dt_2](PV a) { v[a] += dt_2 * dv_dt[a]; });

    equations_.post_integrate(mesh, particles);
    return dt;
  }

private:

  [[no_unique_address]] Equations equations_{};

}; // class VelocityVerletIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Strong-stability-preserving Runge-Kutta order.
enum class SSPRKOrder : std::uint8_t {
  two,
  three,
};

/// Strong-stability-preserving Runge-Kutta time integrator.
template<explicit_equations Equations>
class SSPRKIntegrator final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      Equations::required_fields | TypeSet{r, dr, v, dv_dt, drho_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      Equations::modified_fields | TypeSet{r, v, rho};

  /// Construct time integrator.
  constexpr explicit SSPRKIntegrator(
      Equations equations,
      SSPRKOrder order = SSPRKOrder::three) noexcept
      : equations_{std::move(equations)}, order_{order} {}

  /// Make a step in time.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto step(ParticleMesh& mesh, ParticleArray& particles) const
      -> particle_num_t<ParticleArray> {
    TIT_PROFILE_SECTION("SSPRKIntegrator::step()");
    const auto old_particles(particles);
    const auto dt = substep_(mesh, particles);

    using Num = particle_num_t<ParticleArray>;
    switch (order_) {
      case SSPRKOrder::two:
        substep_(mesh, particles, dt);
        lincomb_(old_particles, particles, Num{1.0 / 2.0});
        break;
      case SSPRKOrder::three:
        substep_(mesh, particles, dt);
        lincomb_(old_particles, particles, Num{1.0 / 4.0});
        substep_(mesh, particles, dt);
        lincomb_(old_particles, particles, Num{2.0 / 3.0});
        break;
      default: std::unreachable();
    }

    equations_.post_integrate(mesh, particles);
    return dt;
  }

private:

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto substep_(ParticleMesh& mesh,
                ParticleArray& particles,
                std::optional<particle_num_t<ParticleArray>> dt =
                    std::nullopt) const -> particle_num_t<ParticleArray> {
    using PV = ParticleView<ParticleArray>;

    equations_.prepare(mesh, particles);
    if (!dt.has_value()) dt = equations_.compute_time_step(particles);
    const auto dt_ = dt.value();

    equations_.compute_continuity(mesh, particles);
    equations_.compute_momentum(mesh, particles);

    par::for_each(particles.fluid(), [dt_](PV a) {
      r[a] += dt_ * v[a];
      v[a] += dt_ * dv_dt[a];
      rho[a] += dt_ * drho_dt[a];
    });

    return dt_;
  }

  template<particle_array<required_fields> ParticleArray>
  static void lincomb_(const ParticleArray& particles,
                       ParticleArray& out_particles,
                       particle_num_t<ParticleArray> weight) {
    using PV = ParticleView<ParticleArray>;
    par::for_each(out_particles.fluid(), [weight, &particles](PV out_a) {
      const auto a = particles[out_a.index()];
      r[out_a] = (1 - weight) * r[a] + weight * r[out_a];
      v[out_a] = (1 - weight) * v[a] + weight * v[out_a];
      rho[out_a] = (1 - weight) * rho[a] + weight * rho[out_a];
    });
  }

  [[no_unique_address]] Equations equations_;
  SSPRKOrder order_{SSPRKOrder::three};

}; // class SSPRKIntegrator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
