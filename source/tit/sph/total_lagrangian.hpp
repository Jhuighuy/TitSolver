/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph::tlsph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference particle position.
TIT_DEFINE_VECTOR_FIELD(r_0);

/// Reference particle density.
TIT_DEFINE_SCALAR_FIELD(rho_0);

/// Reference renormalization matrix.
TIT_DEFINE_MATRIX_FIELD(L_0);

/// Deformation gradient.
TIT_DEFINE_MATRIX_FIELD(F);

/// Green-Lagrange strain tensor.
TIT_DEFINE_MATRIX_FIELD(E);

/// Piola-Kirchhoff stress tensor.
TIT_DEFINE_MATRIX_FIELD(P);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// St. Venant-Kirchhoff constitutive law.
template<class Num>
class StVenantKirchhoff final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, E, F};

  /// Construct the St. Venant-Kirchhoff constitutive law.
  ///
  /// @param Y  Young's modulus.
  /// @param nu Poisson's ratio.
  constexpr StVenantKirchhoff(Num Y, Num nu) noexcept : Y_{Y}, nu_{nu} {}

  /// Compute stress tensor from strain tensor.
  template<particle_view_n<Num> PV>
  constexpr auto stress_tensor(PV a) const noexcept {
    const auto mu = Y_ / (2 * (1 + nu_));
    const auto lambda = Y_ * nu_ / ((1 + nu_) * (1 - 2 * nu_));
    return 2 * mu * F[a] * E[a] + lambda * tr(E[a]) * F[a];
  }

private:

  Num Y_;
  Num nu_;

}; // class StVenantKirchhoff

/// Neo-Hookean constitutive law.
template<class Num>
class NeoHookean final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, F};

  /// Construct the Neo-Hookean constitutive law.
  ///
  /// @param Y  Young's modulus.
  /// @param nu Poisson's ratio.
  constexpr NeoHookean(Num Y, Num nu) noexcept : Y_{Y}, nu_{nu} {}

  /// Compute stress tensor from deformation gradient.
  template<particle_view_n<Num> PV>
  constexpr auto stress_tensor(PV a) const noexcept {
    const auto mu = Y_ / (2 * (1 + nu_));
    const auto lambda = Y_ * nu_ / ((1 + nu_) * (1 - 2 * nu_));
    const auto F_a_fact = lu(F[a]);
    const auto J_a = F_a_fact->det();
    const auto F_a_inv = F_a_fact->inverse();
    return mu * (F[a] - transpose(F_a_inv)) +
           lambda * log(J_a) * transpose(F_a_inv);
  }

private:

  Num Y_;
  Num nu_;

}; // class NeoHookean

/// Constitutive law of the elastic material.
template<class CL, class Num>
concept constitutive_law = std::same_as<CL, StVenantKirchhoff<Num>> ||
                           std::same_as<CL, NeoHookean<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Elastic equations with fixed kernel width in Total Lagrangian formulation.
template<class Num, constitutive_law<Num> ConstitutiveLaw, kernel<Num> Kernel>
class TLElasticEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields = //
      ConstitutiveLaw::fields |  //
      Kernel::fields |           //
      TypeSet{rho, rho_0, r, r_0, v, dv_dt, L_0, F, E, P};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize elastic equations.
  ///
  /// @param h      Particle width.
  /// @param m      Particle mass.
  /// @param cl     Constitutive law.
  /// @param kernel Kernel.
  constexpr explicit TLElasticEquations(Num h,
                                        Num m,
                                        ConstitutiveLaw cl = {},
                                        Kernel kernel = {}) noexcept
      : h_{h}, m_{m},       //
        cl_{std::move(cl)}, //
        kernel_{std::move(kernel)} {
    TIT_ASSERT(h_ > 0.0, "Particle width must be positive.");
    TIT_ASSERT(m_ > 0.0, "Particle mass must be positive.");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh, particle_array<fields> ParticleArray>
  void index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;

    // In Total Lagrangian SPH reference state is captured just once.
    if (initialized_) return;
    initialized_ = true;
    mesh.update(particles, [this](PV /*a*/) { return kernel_.radius(); });

    // Capture the reference state.
    par::for_each(particles.all(), [](PV a) {
      rho_0[a] = rho[a];
      r_0[a] = r[a];
      L_0[a] = {};
    });

    // Compute kernel gradient renormalization matrix.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_0ab = kernel_.grad(r_0[a, b]);
      const auto V_0a = m_ / rho_0[a];
      const auto V_0b = m_ / rho_0[b];

      const auto L_0_flux = outer(r_0[b, a], grad_W_0ab);
      L_0[a] += V_0b * L_0_flux;
      L_0[b] += V_0a * L_0_flux;
    });

    // Finalize kernel gradient renormalization matrix.
    par::for_each(particles.all(), [](PV a) {
      if (const auto fact = ldl(L_0[a]); fact) L_0[a] = fact->inverse();
      else L_0[a] = eye(L_0[a]); // Should never happen.
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh, particle_array<fields> ParticleArray>
  void setup_boundary(ParticleMesh& /*mesh*/,
                      ParticleArray& /*particles*/) const {
    // Nothing to do.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh, particle_array<fields> ParticleArray>
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& /*particles*/) const {
    // Nothing to do. Everything is computed in `compute_forces()`.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity-related fields.
  template<particle_mesh ParticleMesh, particle_array<fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;

    // Prepare fields.
    par::for_each(particles.all(), [](PV a) {
      // Clear fields.
      clear(a, dv_dt, F);

      // Apply body force (gravity).
      dv_dt[a][1] -= 9.81;
    });

    // Compute deformation gradient.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_0ab = kernel_.grad(r_0[a, b]);
      const auto V_0a = m_ / rho_0[a];
      const auto V_0b = m_ / rho_0[b];

      // Update deformation gradient.
      const auto F_flux = outer(r[b, a], grad_W_0ab);
      F[a] += V_0b * F_flux;
      F[b] += V_0a * F_flux;
    });

    // Finalize deformation gradient and compute density, strain and stress
    // tensors.
    par::for_each(particles.all(), [this](PV a) {
      // Renormalize tensor of deformation gradient.
      F[a] = F[a] * L_0[a];

      // Compute current density.
      const auto J_a = lu(F[a])->det();
      rho[a] = rho_0[a] / J_a;

      // Compute Green-Lagrange strain tensor.
      E[a] = (transpose(F[a]) * F[a] - eye(F[a])) / 2;

      // Compute Piola-Kirchhoff stress tensor.
      P[a] = cl_.stress_tensor(a) * L_0[a];
    });

    // Compute velocity time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_0ab = kernel_.grad(r_0[a, b]);

      // Update velocity time derivative.
      const auto v_flux =
          (P[a] / pow2(rho_0[a]) + P[b] / pow2(rho_0[b])) * grad_W_0ab;
      dv_dt[a] += m_ * v_flux;
      dv_dt[b] -= m_ * v_flux;
    });
  }

private:

  Num h_;
  Num m_;
  [[no_unique_address]] ConstitutiveLaw cl_;
  [[no_unique_address]] Kernel kernel_;
  mutable bool initialized_ = false;

}; // class TLElasticEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph::tlsph
