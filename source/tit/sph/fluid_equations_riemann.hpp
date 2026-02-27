/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <tuple>

#include "tit/core/checks.hpp"
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
template<class Num,
         continuity_equation<Num> ContinuityEquation,
         momentum_equation<Num> MomentumEquation,
         equation_of_state<Num> EquationOfState,
         kernel<Num> Kernel,
         riemann_solver<Num> RiemannSolver,
         reconstruction<Num> Reconstruction>
class FluidEquationsRiemann final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields =   //
      ContinuityEquation::fields | //
      MomentumEquation::fields |   //
      EquationOfState::fields |    //
      Kernel::fields |             //
      RiemannSolver::fields |      //
      Reconstruction::fields |     //
      TypeSet{r, rho, grad_rho, v, grad_v, L};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param h                   Particle width.
  /// @param m                   Particle mass.
  /// @param motion_equation     Motion equation.
  /// @param continuity_equation Continuity equation.
  /// @param eos                 Equation of state.
  /// @param kernel              Kernel.
  /// @param riemann_solver      Riemann solver.
  /// @param reconstruct         Reconstruction scheme.
  constexpr explicit FluidEquationsRiemann(
      Num h,
      Num m,
      ContinuityEquation continuity_equation,
      MomentumEquation momentum_equation,
      EquationOfState equation_of_state,
      Kernel kernel,
      RiemannSolver riemann_solver,
      Reconstruction reconstruction) noexcept
      : h_{h}, m_{m}, //
        continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        equation_of_state_{std::move(equation_of_state)},
        kernel_{std::move(kernel)}, //
        riemann_solver_{std::move(riemann_solver)},
        reconstruction_{std::move(reconstruction)} {
    TIT_ASSERT(h_ > 0.0, "Particle width must be positive.");
    TIT_ASSERT(m_ > 0.0, "Particle mass must be positive.");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array_n<Num, fields> ParticleArray>
  void index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.update(particles, [this](PV /*a*/) { return kernel_.radius(); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array_n<Num, fields> ParticleArray>
  void setup_boundary(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquationsRiemann::setup_boundary()");
    apply_bcs(h_, m_, kernel_, mesh, particles);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute density-related fields.
  template<particle_mesh ParticleMesh,
           particle_array_n<Num, fields> ParticleArray>
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& /*particles*/) const {
    // Nothing to do. Everything is computed in `compute_forces()`.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity-related fields.
  template<particle_mesh ParticleMesh,
           particle_array_n<Num, fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquationsRiemann::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Clean-up fields, compute pressure, sound speed and apply source terms.
    par::for_each(particles.all(), [this](PV a) {
      // Clean-up density and momentum equation fields.
      clear(a, drho_dt, grad_rho, dv_dt, grad_v, L);

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
        const auto V_a = m_ / rho[a];
        const auto V_b = m_ / rho[b];
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
      const auto V_a = m_ / rho[a];
      const auto V_b = m_ / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      // Reconstruct the states on the left and right side of the interface.
      Particle ar{PV::space, TypeSet{rho, p, v, r}};
      Particle br{PV::space, TypeSet{rho, p, v, r}};
      r[ar] = r[a];
      r[br] = r[b];
      std::tie(rho[ar], rho[br]) = reconstruction_(rho, grad_rho, a, b);
      std::tie(v[ar], v[br]) = reconstruction_(v, grad_v, a, b);
      p[ar] = equation_of_state_.pressure(ar);
      p[br] = equation_of_state_.pressure(br);

      // Solve the Riemann problem.
      const auto [p_ast, v_ast] = riemann_solver_(ar, br);

      // Update density time derivative.
      drho_dt[a] += 2 * rho[a] * V_b * dot(v[a] - v_ast, grad_W_ab);
      drho_dt[b] -= 2 * rho[b] * V_a * dot(v[b] - v_ast, grad_W_ab);

      // Update velocity time derivative.
      const auto Pi_ab = momentum_equation_.viscosity()(a, b);
      const auto v_flux = (Pi_ab - 2 * p_ast / (rho[a] * rho[b])) * grad_W_ab;
      dv_dt[a] += m_ * v_flux;
      dv_dt[b] -= m_ * v_flux;
    });
  }

private:

  Num h_;
  Num m_;
  [[no_unique_address]] ContinuityEquation continuity_equation_;
  [[no_unique_address]] MomentumEquation momentum_equation_;
  [[no_unique_address]] EquationOfState equation_of_state_;
  [[no_unique_address]] Kernel kernel_;
  [[no_unique_address]] RiemannSolver riemann_solver_;
  [[no_unique_address]] Reconstruction reconstruction_;

}; // class FluidEquationsRiemann

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
