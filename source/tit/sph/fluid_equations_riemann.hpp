/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <tuple>

#include "tit/core/mat.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/reconstruction.hpp"
#include "tit/sph/riemann_solver.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         equation_of_state EquationOfState,
         kernel Kernel,
         riemann_solver RiemannSolver,
         reconstruction_scheme Reconstruction>
class FluidEquationsRiemann final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =   //
      ContinuityEquation::required_fields | //
      MomentumEquation::required_fields |   //
      EquationOfState::required_fields |    //
      Kernel::required_fields |             //
      RiemannSolver::required_fields |      //
      Reconstruction::required_fields |     //
      TypeSet{h, m, r, rho, grad_rho, v, grad_v, L};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      ContinuityEquation::modified_fields | //
      MomentumEquation::modified_fields |   //
      EquationOfState::modified_fields |    //
      Kernel::modified_fields |             //
      RiemannSolver::modified_fields |      //
      Reconstruction::modified_fields |     //
      TypeSet{rho, drho_dt, grad_rho, v, grad_v, dv_dt, L};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param motion_equation     Motion equation.
  /// @param continuity_equation Continuity equation.
  /// @param eos                 Equation of state.
  /// @param kernel              Kernel.
  /// @param riemann_solver      Riemann solver.
  /// @param reconstruct         Reconstruction scheme.
  constexpr explicit FluidEquationsRiemann(
      ContinuityEquation continuity_equation = {},
      MomentumEquation momentum_equation = {},
      EquationOfState eos = {},
      Kernel kernel = {},
      RiemannSolver riemann_solver = {},
      Reconstruction reconstruction = {}) noexcept
      : continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        eos_{std::move(eos)},       //
        kernel_{std::move(kernel)}, //
        riemann_solver_{std::move(riemann_solver)},
        reconstruction_{std::move(reconstruction)} {}

  /// Reflect the fluid equations.
  constexpr void reflect(this auto& /*self*/, auto& /*refl*/) {
    // Nothing to do at the moment.
  }

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
    TIT_PROFILE_SECTION("FluidEquationsRiemann::setup_boundary()");
    apply_bcs(kernel_, mesh, particles);
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
    TIT_PROFILE_SECTION("FluidEquationsRiemann::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up fields, compute pressure, sound speed and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up density and momentum equation fields.
      clear(a, drho_dt, grad_rho, dv_dt, grad_v, L);

      // Compute pressure.
      p[a] = eos_.pressure(a);

      // Apply source terms.
      std::apply([a](const auto&... f) { ((drho_dt[a] += f(a)), ...); },
                 continuity_equation_.mass_sources());
      std::apply([a](const auto&... g) { ((dv_dt[a] += g(a)), ...); },
                 momentum_equation_.momentum_sources());
    });

    // Compute gradients of the fields.
    if constexpr (has_any<PV>(grad_rho, grad_v, L)) {
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

        // Update velocity gradient.
        if constexpr (has<PV>(grad_v)) {
          const auto grad_flux = outer(v[b, a], grad_W_ab);
          grad_v[a] += V_b * grad_flux;
          grad_v[b] += V_a * grad_flux;
        }

        // Update renormalization matrix.
        if constexpr (has<PV>(L)) {
          const auto L_flux = outer(r[b, a], grad_W_ab);
          L[a] += V_b * L_flux;
          L[b] += V_a * L_flux;
        }
      });

      // Renormalize fields.
      if constexpr (has<PV>(L) && has_any<PV>(grad_rho, grad_v)) {
        par::for_each(particles.all(), [](PV a) {
          if (const auto fact = ldl(L[a]); fact) {
            const auto inv_L = fact->inverse();
            if constexpr (has<PV>(grad_rho)) grad_rho[a] = inv_L * grad_rho[a];
            if constexpr (has<PV>(grad_v)) grad_v[a] = inv_L * grad_v[a];
          } else {
            L[a] = eye(L[a]);
          }
        });
      }
    }

    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto V_a = m[a] / rho[a];
      const auto V_b = m[b] / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      // Reconstruct the states on the left and right side of the interface.
      const auto [ar, br] = [this, a, b] {
        if constexpr (std::same_as<Reconstruction, NoReconstruction>) {
          return std::tuple{a, b};
        } else {
          ParticleView ar_{a, TypeSet{rho, v}};
          ParticleView br_{b, TypeSet{rho, v}};
          std::tie(rho[ar_], rho[br_]) = reconstruction_(rho, grad_rho, a, b);
          std::tie(v[ar_], v[br_]) = reconstruction_(v, grad_v, a, b);
          return std::tuple{ar_, br_};
        }
      }();

      // Compute the pressure at the interface.
      ParticleView ap{ar, TypeSet{p}};
      ParticleView bp{br, TypeSet{p}};
      p[ap] = eos_.pressure(ap);
      p[bp] = eos_.pressure(bp);

      // Solve the Riemann problem.
      const auto [p_ast, v_ast] = riemann_solver_(ap, bp);

      // Update density time derivative.
      drho_dt[a] += 2 * rho[a] * V_b * dot(v[a] - v_ast, grad_W_ab);
      drho_dt[b] -= 2 * rho[b] * V_a * dot(v[b] - v_ast, grad_W_ab);

      // Update velocity time derivative.
      const auto Pi_ab = momentum_equation_.viscosity()(a, b);
      const auto v_flux = (Pi_ab - 2 * p_ast / (rho[a] * rho[b])) * grad_W_ab;
      dv_dt[a] += m[b] * v_flux;
      dv_dt[b] -= m[a] * v_flux;
    });
  }

private:

  [[no_unique_address]] ContinuityEquation continuity_equation_;
  [[no_unique_address]] MomentumEquation momentum_equation_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;
  [[no_unique_address]] RiemannSolver riemann_solver_;
  [[no_unique_address]] Reconstruction reconstruction_;

}; // class FluidEquationsRiemann

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
