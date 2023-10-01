/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/par/algorithms.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/riemann_solver.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<continuity_equation ContinuityEquation,
         momentum_equation MomentumEquation,
         equation_of_state EquationOfState,
         riemann_solver RiemannSolver,
         kernel Kernel>
class FluidEquationsRiemann final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =   //
      ContinuityEquation::required_fields | //
      MomentumEquation::required_fields |   //
      EquationOfState::required_fields |    //
      RiemannSolver::required_fields |      //
      Kernel::required_fields | TypeSet{h, m, r, rho, p, v, dv_dt};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      ContinuityEquation::modified_fields | //
      MomentumEquation::modified_fields |   //
      EquationOfState::modified_fields |    //
      Kernel::modified_fields |             //
      RiemannSolver::modified_fields |      //
      TypeSet{rho, drho_dt, grad_rho, p, v, dv_dt};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param motion_equation     Motion equation.
  /// @param continuity_equation Continuity equation.
  /// @param eos                 Equation of state.
  /// @param riemann_solver      Riemann solver.
  /// @param kernel              Kernel.
  constexpr explicit FluidEquationsRiemann(
      ContinuityEquation continuity_equation = {},
      MomentumEquation momentum_equation = {},
      EquationOfState eos = {},
      RiemannSolver riemann_solver = {},
      Kernel kernel = {}) noexcept
      : continuity_equation_{std::move(continuity_equation)},
        momentum_equation_{std::move(momentum_equation)},
        riemann_solver_{std::move(riemann_solver)}, //
        eos_{std::move(eos)},                       //
        kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_array<required_fields> ParticleArray>
  constexpr void init(ParticleArray& /*particles*/) const {
    // Nothing to do.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.update(particles, [this](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Setup boundary particles.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void setup_boundary(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquationsRiemann::setup_boundary()");
    using PV = ParticleView<ParticleArray>;
    using Num = particle_num_t<ParticleArray>;
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
        clear(b, rho, v, u);
        const auto E = fact->solve(unit<0>(M[0]));
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
          const auto W_delta = dot(E, B_delta) * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
          if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
        }
      } else if (!is_tiny(S)) {
        // Constant interpolation succeeds, use it.
        clear(b, rho, v, u);
        const auto E = inverse(S);
        for (const PV a : mesh.fixed_interp(b)) {
          const auto r_delta = r_ghost - r[a];
          const auto W_delta = E * kernel_(r_delta, h_ghost);
          rho[b] += m[a] * W_delta;
          v[b] += m[a] / rho[a] * v[a] * W_delta;
          if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
        }
      } else {
        // Both interpolations fail, leave the particle as it is.
        return;
      }

      // Compute the density at the boundary.
      // drho/dn = rho_0/(cs_0^2)*dot(g,n).
      constexpr Num rho_0 = 1000.0;
      constexpr Num cs_0 = 20 * sqrt(9.81 * 0.6);
      constexpr Vec<Num, 2> G{0.0, -9.81};
      rho[b] += SD * rho_0 / pow2(cs_0) * dot(G, SN);

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
  constexpr void compute_density(ParticleMesh& /*mesh*/,
                                 ParticleArray& /*particles*/) const {
    // Nothing to do. Everything is computed in `compute_forces()`.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute velocity related fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_forces(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquationsRiemann::compute_density()");
    using PV = ParticleView<ParticleArray>;

    // Prepare the fields and setup sources and pressure.
    par::for_each(particles.all(), [&](PV a) {
      // Clean-up density and momentum equation fields.
      clear(a, drho_dt, dv_dt);

      // Apply source terms.
      std::apply([a](const auto&... f) { ((drho_dt[a] += f(a)), ...); },
                 continuity_equation_.mass_sources());
      std::apply([a](const auto&... g) { ((dv_dt[a] += g(a)), ...); },
                 momentum_equation_.momentum_sources());

      // Compute pressure and sound speed.
      p[a] = eos_.pressure(a);
    });

    // Compute velocity and internal energy time derivatives.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto V_a = m[a] / rho[a];
      const auto V_b = m[b] / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      // Solve the Riemann problem.
      const auto [p_ast, v_ast] = riemann_solver_(a, b);

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
  [[no_unique_address]] RiemannSolver riemann_solver_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquationsRiemann

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
