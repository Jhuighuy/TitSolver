/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <limits>
#include <numbers>
#include <ranges>

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/bcs.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/motion_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<motion_equation MotionEquation,
         continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         energy_equation EnergyEquation,
         equation_of_state EquationOfState,
         kernel Kernel>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =   //
      MotionEquation::required_fields |     //
      ContinuityEquation::required_fields | //
      MomentumEquation::required_fields |   //
      EnergyEquation::required_fields |     //
      EquationOfState::required_fields |    //
      Kernel::required_fields | TypeSet{h, m, r, rho, p, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      MotionEquation::modified_fields |       //
      ContinuityEquation::modified_fields |   //
      MomentumEquation::modified_fields |     //
      EnergyEquation::modified_fields |       //
      EquationOfState::modified_fields |      //
      Kernel::modified_fields |               //
      TypeSet{rho, drho_dt, grad_rho, N, L} | //
      TypeSet{p, v, dv_dt} |                  //
      TypeSet{u, du_dt} |                     //
      TypeSet{dr, FS};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param motion_equation     Motion equation.
  /// @param continuity_equation Continuity equation.
  /// @param momentum_equation   Momentum equation.
  /// @param energy_equation     Energy equation.
  /// @param equation_of_state   Equation of state.
  /// @param kernel              Kernel.
  constexpr explicit FluidEquations(MotionEquation motion_equation,
                                    ContinuityEquation continuity_equation,
                                    MomentumEquation momentum_equation,
                                    EnergyEquation energy_equation,
                                    EquationOfState eos,
                                    Kernel kernel) noexcept
      : motion_equation_{std::move(motion_equation)},
        continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        energy_equation_{std::move(energy_equation)}, //
        eos_{std::move(eos)},                         //
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.update(particles, [this](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void setup_boundary(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::setup_boundary()");
    apply_bcs(kernel_, mesh, particles);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_density(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up continuity equation fields and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up continuity equation fields.
      clear(a, drho_dt, grad_rho, N, L);

      // Apply continuity equation source terms.
      std::apply([a](const auto&... f) { ((drho_dt[a] += f(a)), ...); },
                 continuity_equation_.mass_sources());
    });

    // Compute density gradient and renormalization fields.
    if constexpr (has_any<PV>(grad_rho, N, L)) {
      // Precompute the fields.
      par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
        const auto [a, b] = ab;
        const auto V_a = m[a] / rho[a];
        const auto V_b = m[b] / rho[b];
        const auto grad_W_ab = kernel_.grad(a, b);

        // Update density gradient.
        if constexpr (has<PV>(grad_rho)) {
          const auto grad_flux = rho[b, a] * grad_W_ab;
          grad_rho[a] += V_b * grad_flux;
          grad_rho[b] += V_a * grad_flux;
        }

        // Update normal vector.
        if constexpr (has<PV>(N)) {
          N[a] += V_b * grad_W_ab;
          N[b] -= V_a * grad_W_ab;
        }

        // Update renormalization matrix.
        if constexpr (has<PV>(L)) {
          const auto L_flux = outer(r[b, a], grad_W_ab);
          L[a] += V_b * L_flux;
          L[b] += V_a * L_flux;
        }
      });

      // Renormalize fields.
      par::for_each(particles.all(), [](PV a) {
        // Renormalize density gradient and normal vector, if possible.
        if constexpr (has<PV>(L) && has_any<PV>(N, grad_rho)) {
          if (const auto fact = ldl(L[a]); fact) {
            if constexpr (has<PV>(N)) N[a] = fact->solve(N[a]);
            if constexpr (has<PV>(grad_rho))
              grad_rho[a] = fact->solve(grad_rho[a]);
          } else {
            L[a] = eye(L[a]);
          }
        }

        // Finalize the normal vector.
        if constexpr (has<PV>(N)) N[a] = normalize(N[a]);
      });
    }

    // Compute density time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);

      // Update density time derivative.
      const auto Psi_ab =
          momentum_equation_.artificial_viscosity().density_term(a, b);
      drho_dt[a] -= m[b] * dot(v[b, a] - Psi_ab / rho[b], grad_W_ab);
      drho_dt[b] -= m[a] * dot(v[b, a] + Psi_ab / rho[a], grad_W_ab);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_forces()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up momentum and energy equation fields, compute pressure,
    // sound speed and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up momentum and energy equation fields.
      clear(a, dv_dt, du_dt);

      // Apply source terms.
      std::apply(
          [a](const auto&... g) {
            ((dv_dt[a] += g(a)), ...);
            if constexpr (has<PV>(du_dt)) ((du_dt[a] += dot(g(a), v[a])), ...);
          },
          momentum_equation_.momentum_sources());
      if constexpr (has<PV>(du_dt)) {
        std::apply([a](const auto&... q) { ((du_dt[a] += q(a)), ...); },
                   energy_equation_.energy_sources());
      }

      // Compute pressure and sound speed.
      p[a] = eos_.pressure(a);
      if constexpr (has<PV>(cs)) cs[a] = eos_.sound_speed(a);
    });

    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);

      // Update velocity time derivative.
      const auto P_a = p[a] / pow2(rho[a]);
      const auto P_b = p[b] / pow2(rho[b]);
      const auto Pi_ab =
          momentum_equation_.viscosity()(a, b) +
          momentum_equation_.artificial_viscosity().velocity_term(a, b);
      const auto v_flux = (-P_a - P_b + Pi_ab) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;

      // Update internal energy time derivative.
      if constexpr (has<PV>(du_dt)) {
        const auto Q_ab = energy_equation_.heat_conductivity()(a, b);
        du_dt[a] -= m[b] * dot((P_a - Pi_ab / 2) * v[b, a] - Q_ab, grad_W_ab);
        du_dt[b] -= m[a] * dot((P_b - Pi_ab / 2) * v[b, a] + Q_ab, grad_W_ab);
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute particle shifts.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_shifts(ParticleMesh& mesh,
                                ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_shifts()");
    using PV = ParticleView<ParticleArray>;
    using Num = particle_num_t<PV>;

    const auto R = motion_equation_.particle_shifting().R();
    const auto Ma = motion_equation_.particle_shifting().Ma();
    const auto CFL = motion_equation_.particle_shifting().CFL();

    // Initialize the free surface flag values and clear the particle shifts.
    // - Positive value `FS_FAR` means that the particle is far from the free
    //   surface.
    // - Any positive value in the range `(FS_FAR, FS_ON)` means that the
    //   particle is near the free surface (has at least one neighbor that is on
    //   the free surface).
    // - Value `FS_ON` means that the particle is on the free surface. It is
    //   essentially zero, but we use a very small number to avoid spurious
    //   comparisons.
    const auto a_0 = particles[0];
    const auto FS_FAR = 2 * CFL * Ma * pow2(h[a_0]);
    static constexpr auto FS_ON = std::numeric_limits<Num>::min();
    par::for_each(particles.fluid(), [](PV a) { FS[a] = FS_ON, dr[a] = {}; });
    par::for_each(particles.fixed(), [FS_FAR](PV a) { FS[a] = FS_FAR; });

    // Classify the particles into free surface and non-free surface.
    //
    // Here we are reading and writing the same field `FS` in the parallel loop.
    // There is no race condition because we read the neighbor to compare it
    // with `FS_ON`, and non-free-surface particles are updated in the loop.
    par::block_for_each(mesh.block_pairs(particles), [FS_FAR](auto ab) {
      const auto [a, b] = ab;

      // Skip the particles that are too far away.
      const auto r_ab = norm2(r[a, b]);
      if (const auto dist_threshold = pow2(2 * h[a]); r_ab > dist_threshold) {
        return;
      }

      // Perform "visibility" test. The actual test is just an optimized
      // version of `acos(n_{a,b} / sqrt(r_ab)) <= fov`.
      constexpr auto cos_fov = static_cast<Num>(cos(std::numbers::pi / 4));
      const auto fov_threshold = cos_fov * r_ab;
      if (bitwise_equal(FS[a], FS_ON)) {
        const auto n_a = dot(N[a], r[a, b]);
        if (n_a > 0 && pow2(n_a) >= fov_threshold) FS[a] = FS_FAR;
      }
      if (bitwise_equal(FS[b], FS_ON)) {
        const auto n_b = dot(N[b], r[a, b]);
        if (n_b < 0 && pow2(n_b) >= fov_threshold) FS[b] = FS_FAR;
      }
    });

    // Classify the non-free surface particles into near and far categories.
    //
    // Here we are reading and writing the same field `FS` in the parallel loop.
    // There is no race condition because we update the field only when
    // the particle has `FS_ON`, and read the field only to compare it with
    // `FS_ON`.
    //
    // A distinct non-zero bit pattern of `FS_ON` is essential for
    // correctness. We may read a garbage value while memory is being updated by
    // some other thread, and the chances of a false positive comparison with
    // distinct bits are very small, at least orders of magnitude smaller than
    // if we used zero.
    par::for_each(particles.fluid(), [FS_FAR, &mesh, this](PV a) {
      if (!bitwise_equal(FS[a], FS_FAR)) return;

      // Do not apply the shifts to the particles near the walls.
      /// @todo No article mentions this. We shall investigate it.
      if (std::ranges::any_of(mesh[a], [](PV b) { return b.is_fixed(); })) {
        FS[a] = Num{1.0e-30} * FS_FAR;
        return;
      }

      constexpr auto on_fs = [](PV b) { return bitwise_equal(FS[b], FS_ON); };
      if (std::ranges::any_of(mesh[a], on_fs)) {
        auto fs_neighbors = std::views::filter(mesh[a], on_fs);
        const auto dist_to_a = [a](PV b) { return norm2(r[a, b]); };
        const auto b = *std::ranges::min_element(fs_neighbors, {}, dist_to_a);
        FS[a] *= abs(dot(N[b], r[a, b])) / kernel_.radius(a);
      }
    });

    // Compute the particle shifts.
    const auto inv_W_0 = inverse(kernel_(unit(r[a_0], h[a_0] / 2), h[a_0]));
    par::block_for_each(
        mesh.block_pairs(particles),
        [inv_W_0, FS_FAR, R, this](auto ab) {
          const auto [a, b] = ab;
          const auto W_ab = kernel_(a, b);
          const auto grad_W_ab = kernel_.grad(a, b);

          // Update the particle shifts.
          const auto Chi_ab = R * pow<4>(W_ab * inv_W_0);
          const auto Xi_a = static_cast<Num>(bitwise_equal(FS[a], FS_FAR));
          const auto Xi_b = static_cast<Num>(bitwise_equal(FS[b], FS_FAR));
          dr[a] -= (Xi_a + Chi_ab) * FS[a] * m[b] / rho[b] * grad_W_ab;
          dr[b] += (Xi_b + Chi_ab) * FS[b] * m[a] / rho[a] * grad_W_ab;
        });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] MotionEquation motion_equation_;
  [[no_unique_address]] ContinuityEquation continuity_equation_;
  [[no_unique_address]] MomentumEquation momentum_equation_;
  [[no_unique_address]] EnergyEquation energy_equation_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
