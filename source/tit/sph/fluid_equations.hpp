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
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<class Num,
         equation_of_state<Num> EquationOfState,
         artificial_viscosity<Num> ArtificialViscosity,
         kernel Kernel>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =    //
      EquationOfState::required_fields |     //
      ArtificialViscosity::required_fields | //
      Kernel::required_fields |
      TypeSet{h, m, r, dr, rho, p, v, dv_dt, L, N, FS};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      EquationOfState::modified_fields |      //
      ArtificialViscosity::modified_fields |  //
      Kernel::modified_fields |               //
      TypeSet{rho, drho_dt, grad_rho, N, L} | //
      TypeSet{p, v, dv_dt} |                  //
      TypeSet{dr, FS};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param rho_0                Reference density.
  /// @param cs_0                 Reference sound speed.
  /// @param g                    Gravitational acceleration.
  /// @param R                    Kernel support radius scale factor.
  /// @param Ma                   Mach number.
  /// @param CFL                  CFL number.
  /// @param equation_of_state    Equation of state.
  /// @param artificial_viscosity Artificial viscosity.
  /// @param kernel               Kernel.
  constexpr explicit FluidEquations(Num rho_0,
                                    Num cs_0,
                                    Num g,
                                    Num R,
                                    Num Ma,
                                    Num CFL,
                                    EquationOfState eos,
                                    ArtificialViscosity artificial_viscosity,
                                    Kernel kernel) noexcept
      : rho_0_{rho_0}, cs_0_{cs_0}, g_{g}, R_{R}, Ma_{Ma}, CFL_{CFL}, //
        eos_{std::move(eos)},
        artificial_viscosity_{std::move(artificial_viscosity)},
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
    using PV = ParticleView<ParticleArray>;
    static constexpr auto Dim = particle_dim_v<ParticleArray>;

    // Interpolate the field values on the boundary.
    par::for_each(particles.fixed(), [this, &mesh](PV b) {
      /// @todo Once we have a proper geometry library, we should use
      ///       here and clean up the code.
      const auto& search_point = r[b];
      const auto clipped_point = Domain<Num>.clamp(search_point);
      const auto r_ghost = 2 * clipped_point - search_point;
      const auto SN = normalize(search_point - clipped_point);
      const auto SD = norm(r_ghost - r[b]);

      // Compute the interpolation weights, both for the constant and
      // linear interpolations.
      Num S{};
      Mat<Num, Dim + 1> M{};
      const auto h_ghost = RADIUS_SCALE * h[b];
      for (const PV a : mesh.fixed_interp(b)) {
        const auto r_delta = r_ghost - r[a];
        const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
        const auto W_delta = kernel_(r_delta, h_ghost);
        S += W_delta * m[a] / rho[a];
        M += outer(B_delta, B_delta * W_delta * m[a] / rho[a]);
      }

      if (const auto fact = ldl(M); fact) {
        // Linear interpolation succeeds, use it.
        clear(b, rho, v);
        const auto E = fact->solve(unit<0>(M[0]));
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
          const auto W_delta = dot(E, B_delta) * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
        }
      } else if (!is_tiny(S)) {
        // Constant interpolation succeeds, use it.
        clear(b, rho, v);
        const auto E = inverse(S);
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto W_delta = E * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
        }
      } else {
        // Both interpolations fail, leave the particle as it is.
        rho[b] = rho_0_;
        v[b] = {};
        return;
      }

      // Compute the density at the boundary.
      // drho/dn = rho_0/(cs_0^2)*dot(g,n).
      const auto G = unit<1>(r[b], -g_);
      rho[b] += SD * rho_0_ / pow2(cs_0_) * dot(G, SN);

      // Compute the velocity at the boundary (slip wall boundary condition).
      const auto Vn = dot(v[b], SN) * SN;
      const auto Vt = v[b] - Vn;
      v[b] = Vt - Vn;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_density(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up continuity equation fields.
    par::for_each(particles.all(), [](PV a) {
      drho_dt[a] = {};
      grad_rho[a] = {};
      L[a] = {};
      N[a] = {};
    });

    // Compute density gradient and normalization fields.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;

      // Compute volumes and kernel gradient.
      const auto V_a = m[a] / rho[a];
      const auto V_b = m[b] / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      // Update density gradient.
      const auto grad_flux = rho[b, a] * grad_W_ab;
      grad_rho[a] += V_b * grad_flux;
      grad_rho[b] += V_a * grad_flux;

      // Update normal vector.
      const auto N_flux = grad_W_ab;
      N[a] += V_b * N_flux;
      N[b] -= V_a * N_flux;

      // Update normalization matrix.
      const auto L_flux = outer(r[b, a], grad_W_ab);
      L[a] += V_b * L_flux;
      L[b] += V_a * L_flux;
    });

    // Finalize the normalization fields.
    par::for_each(particles.all(), [](PV a) {
      // If normalization matrix is invertible, invert it.
      if (const auto fact = ldl(L[a])) {
        L[a] = fact->inverse();
      } else {
        L[a] = eye(L[a]);
      }

      // Apply the normalization matrix.
      grad_rho[a] = L[a] * grad_rho[a];
      N[a] = normalize(L[a] * N[a]);
    });

    // Compute density time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;

      // Compute the kernel gradient.
      const auto grad_W_ab = kernel_.grad(a, b);

      // Compute the artificial viscosity density term.
      const auto Psi_ab = artificial_viscosity_.density_term(a, b);

      // Update density time derivative.
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

    // Initialize the momentum and compute pressure.
    par::for_each(particles.all(), [this](PV a) {
      // Initialize the momentum time derivative with the gravity source.
      dv_dt[a] = unit<1>(r[a], -g_);

      // Compute pressure and sound speed.
      p[a] = eos_.pressure(a);
      if constexpr (has<PV>(cs)) cs[a] = eos_.sound_speed(a);
    });

    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;

      // Compute the kernel gradient.
      const auto grad_W_ab = kernel_.grad(a, b);

      // Compute the artificial viscosity velocity term.
      const auto Pi_ab = artificial_viscosity_.velocity_term(a, b);

      // Update momentum time derivative.
      const auto P_a = p[a] / pow2(rho[a]);
      const auto P_b = p[b] / pow2(rho[b]);
      const auto v_flux = (-P_a - P_b + Pi_ab) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;
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
    const auto FS_FAR = 2 * CFL_ * Ma_ * pow2(h[a_0]);
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
        [inv_W_0, FS_FAR, this](auto ab) {
          const auto [a, b] = ab;
          const auto W_ab = kernel_(a, b);
          const auto grad_W_ab = kernel_.grad(a, b);

          // Update the particle shifts.
          const auto Chi_ab = R_ * pow<4>(W_ab * inv_W_0);
          const auto Xi_a = static_cast<Num>(bitwise_equal(FS[a], FS_FAR));
          const auto Xi_b = static_cast<Num>(bitwise_equal(FS[b], FS_FAR));
          dr[a] -= (Xi_a + Chi_ab) * FS[a] * m[b] / rho[b] * grad_W_ab;
          dr[b] += (Xi_b + Chi_ab) * FS[b] * m[a] / rho[a] * grad_W_ab;
        });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Num rho_0_;
  Num cs_0_;
  Num g_;
  Num R_;
  Num Ma_;
  Num CFL_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] ArtificialViscosity artificial_viscosity_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
