/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/type.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph::fsi {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference particle position.
TIT_DEFINE_VECTOR_FIELD(r_0);

/// Piola-Kirchhoff stress tensor.
TIT_DEFINE_MATRIX_FIELD(P);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Hooke’s law equation of state.
template<class Num>
class HookesLaw final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct the equation of state.
  ///
  /// @param E_s  Young's modulus.
  /// @param nu_s Poisson's ratio.
  constexpr HookesLaw(Num E_s, Num nu_s) noexcept : E_s_{E_s}, nu_s_{nu_s} {}

  /// Compute sound speed.
  template<particle_view_n<Num> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    const auto K_s = E_s_ / (Num{3} * (Num{1} - Num{2} * nu_s_));
    const auto cs_0 = sqrt(K_s / rho[a]);
    return cs_0;
  }

  /// Compute Cauchy stress tensor from Euler strain tensor.
  template<size_t Dim>
  constexpr auto stress_tensor(const Mat<Num, Dim>& eps) const noexcept {
    static_assert(Dim == 2);
    Mat<Num, Dim> sigma;
    sigma[0, 0] = eps[0, 0] + nu_s_ * eps[1, 1];
    sigma[1, 1] = nu_s_ * eps[0, 0] + eps[1, 1];
    sigma[0, 1] = (1 - nu_s_) * eps[0, 1];
    sigma[1, 0] = sigma[0, 1];
    return std::tuple{E_s_ / (1.0 - pow2(nu_s_)), sigma};
  }

private:

  Num E_s_;
  Num nu_s_;

}; // class HookesLaw

/// Equation of state type.
template<class EOS>
concept equation_of_state = specialization_of<EOS, HookesLaw>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The particle estimator with a fixed kernel width.
template<equation_of_state EquationOfState,
         artificial_viscosity ArtificialViscosity,
         kernel Kernel>
class ElasticEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =    //
      EquationOfState::required_fields |     //
      ArtificialViscosity::required_fields | //
      Kernel::required_fields |              //
      TypeSet{h, m, rho, P, cs, r, r_0, v, dv_dt, L};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      EquationOfState::modified_fields |     //
      ArtificialViscosity::modified_fields | //
      Kernel::modified_fields |              //
      TypeSet{rho, P, cs, r, r_0, v, dv_dt, L};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize elastic equations.
  ///
  /// @param eos     Equation of state.
  /// @param artvisc Artificial viscosity.
  /// @param kernel  Kernel.
  constexpr explicit ElasticEquations(EquationOfState eos = {},
                                      ArtificialViscosity artvisc = {},
                                      Kernel kernel = {}) noexcept
      : eos_{std::move(eos)},         //
        artvisc_{std::move(artvisc)}, //
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;

    // In Total Lagrangian SPH reference state is captured just once.
    if (initialized_) return;
    initialized_ = true;
    mesh.update(particles, [this](PV a) { return kernel_.radius(a); });

    // Store the reference state.
    par::for_each(particles.all(), [](PV a) {
      // Store initial particle positions.
      r_0[a] = r[a];

      // Clear the renomalization matrix.
      clear(a, L);
    });

    // Compute kernel gradient renormalization matrix.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      const auto V0_a = m[a] / rho[a];
      const auto V0_b = m[b] / rho[b];

      const auto L_flux = outer(r_0[b, a], grad0_W_ab);
      L[a] += V0_b * L_flux;
      L[b] += V0_a * L_flux;
    });

    // Finalize kernel gradient renormalization matrix.
    par::for_each(particles.all(), [](PV a) {
      const auto fact = lu(L[a]);
      if (fact) L[a] = transpose(fact->inverse());
      else L[a] = eye(L[a]);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void setup_boundary(ParticleMesh& /*mesh*/,
                      ParticleArray& /*particles*/) const {
    // Nothing to do.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& /*particles*/) const {
    // Nothing to do. Everything is computed in `compute_forces()`.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity-related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;

    // Prepare velocity-related fields.
    par::for_each(particles.all(), [this](PV a) {
      // Clear velocity-related fields.
      clear(a, dv_dt, P);

      // Apply body force (gravity).
      dv_dt[a][1] -= 9.81;

      // Compute sound speed.
      cs[a] = eos_.sound_speed(a);
    });

    // Compute tensor of deformation gradient and artificial viscous force.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);
      const auto V0_a = m[a] / rho[a];
      const auto V0_b = m[b] / rho[b];

      // Update tensor of deformation gradient (stored in `P`).
      const auto P_flux = outer(r[b, a], grad0_W_ab);
      P[a] += V0_b * P_flux;
      P[b] += V0_a * P_flux;

      // Update artificial viscous force.
      const auto Pi_ab = artvisc_.velocity_term(a, b);
      const auto v_flux = Pi_ab * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;
    });

    par::for_each(particles.all(), [this](PV a) {
      // Finalize tensor of deformation gradient (stored in `P`)
      // and compute auxiliary tensors from it.
      const auto F_a = P[a] * L[a];
      const auto F_T_a = transpose(F_a);
      const auto F_T_a_fact = lu(F_T_a);
      const auto F_invT_a = F_T_a_fact->inverse();
      const auto J_a = F_T_a_fact->det();

      // Compute Green-Lagrange strain tensor.
      const auto E_a = 0.5 * (F_T_a * F_a - eye(F_a));

      // Compute Euler strain tensor.
      const auto eps_a = F_invT_a * E_a * F_T_a;

      // Cauchy stress tensor.
      auto [weight_a, sigma_a] = eos_.stress_tensor(eps_a);
      sigma_a *= weight_a / pow2(rho[a]);

      // Compute Piola-Kirchhoff stress tensor.
      P[a] = J_a * sigma_a * F_invT_a;

      // Finalize artificial viscous force.
      dv_dt[a] = J_a * F_invT_a * dv_dt[a];
    });

    // Compute velocity time derivative.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad0_W_ab = kernel_.grad(r_0[a, b], h[a]);

      // Update velocity time derivative.
      const auto v_flux = (P[a] + P[b]) * grad0_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;
    });
  }

private:

  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] ArtificialViscosity artvisc_;
  [[no_unique_address]] Kernel kernel_;
  mutable bool initialized_ = false;

}; // class StructureEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph::fsi
