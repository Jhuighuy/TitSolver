/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numbers>
#include <ranges>
#include <utility>

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/exchange.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<class Num,
         equation_of_state<Num> EquationOfState,
         kernel Kernel,
         class Domain,
         class Containment>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = //
      TypeSet{h, m, gamma, grad_gamma, rho, drho_dt, grad_rho, p, cs} |
      TypeSet{v, dv_dt, grad_v, r, dr, L, N, phi, rho_raw};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      TypeSet{m, gamma, grad_gamma, rho, drho_dt, grad_rho, p, cs} |
      TypeSet{v, dv_dt, grad_v, r, dr, N, L, phi, rho_raw};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param rho_0       Reference density.
  /// @param g           Gravitational acceleration.
  /// @param mu          Viscosity coefficient.
  /// @param domain      Boundary domain.
  /// @param containment Domain containment function.
  /// @param eos         Equation of state.
  /// @param kernel      Kernel.
  explicit FluidEquations(Num g,
                          Num mu,
                          const Domain& domain,
                          const Containment& containment,
                          EquationOfState eos,
                          Kernel kernel)
      : g_{g}, mu_{mu},                             //
        domain_{domain}, containment_{containment}, //
        eos_{std::move(eos)}, kernel_{std::move(kernel)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Initialization steps.
  //

  /// Initialize equation-owned particle fields.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  void initialize(ParticleMesh& mesh,
                  ParticleArray& particles,
                  Exchange& x = serial_exchange) const {
    TIT_PROFILE_SECTION("FluidEquations::initialize()");
    using PV = ParticleView<ParticleArray>;

    // Compute gamma.
    index(mesh, particles);
    compute_gamma(mesh, particles);

    // Fixed particles have mass fractions.
    par::for_each(particles.fixed(), [](PV a) { m[a] *= gamma[a]; });
    x.refresh(particles, TypeSet{m});
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Preparation steps.
  //

  /// Refresh mesh and boundary state before evaluating equations.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  void prepare(ParticleMesh& mesh,
               ParticleArray& particles,
               Exchange& x = serial_exchange) const {
    TIT_PROFILE_SECTION("FluidEquations::prepare()");

    // The preceding phases have advanced the state of the owned particles.
    x.refresh(particles, TypeSet{r, v, rho});

    index(mesh, particles);
    compute_gamma(mesh, particles);

    // Boundary extrapolation updates the density of the owned wall particles.
    setup_boundary(mesh, particles);
    x.refresh(particles, TypeSet{rho});
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Update the particle mesh.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void index(ParticleMesh& mesh, ParticleArray& particles) const {
    using PV = ParticleView<ParticleArray>;
    mesh.update(domain_, particles, [this](PV a) { return kernel_.radius(a); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Extrapolate the near-wall particles into the boundary.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void setup_boundary(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::setup_boundary()");
    using PV = ParticleView<ParticleArray>;

    // Reconstruct the state on the wall particles from nearby fluid.
    par::for_each(particles.fixed(), [this, &mesh](PV e) {
      // No-slip the velocity on the wall.
      v[e] = {};

      // Neumann equation for density.
      //
      //   ∂p/∂n = ρg·n ↔ cs^2/ρ ∂ρ/∂n = g·n.
      //
      // Define:
      //
      //   H(ρ) := ∫ cs^2(ζ)/ζ dζ from ρ_0 to ρ.
      //   → H(ρ) = cs_0^2 log(ρ / ρ_0) if ξ = 1,
      //   → H(ρ) = cs_0^2/(ξ - 1) [(ρ / ρ_0)^(ξ - 1) - 1] if ξ > 1.
      //
      // Since g·n = ∇(g·r)·n = ∂/∂n (g·r) , the Neumann equation can be
      // rewritten as:
      //
      //   ∂/∂n (H(ρ) + g·r) = 0.
      //
      // Evaluate H(ρ) + g·r via Ferrand style extrapolation, solve for ρ.
      Num S_e{};
      Num H_e{};
      const auto n_e = normalize(grad_gamma[e]);
      for (const PV b : mesh[e]) {
        if (!b.is_fluid()) continue;

        const auto V_b = m[b] / rho[b];
        const auto r_be = r[b] - r[e];
        const auto W_be = kernel_(r_be, h[e]);

        const auto H_b = eos_.potential_from_density(rho[b]);

        S_e += V_b * W_be;
        H_e += V_b * (H_b + g_ * dot(r_be, n_e) * n_e[1]) * W_be;
      }
      rho[e] = eos_.density_from_potential(is_tiny(S_e) ? Num{0} : H_e / S_e);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute gamma and its gradient.
  ///
  /// Only the owned particles are processed: the ghost values are never
  /// read (the pair interactions divide by the gamma of the particle they
  /// update, and the ghost updates are discarded), and the boundary faces
  /// adjacent to a ghost particle may refer to vertices beyond the halo.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_gamma(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_gamma_gradient()");
    using PV = ParticleView<ParticleArray>;

    par::for_each(particles.owned(), [&mesh, this](PV a) {
      // Compute gamma gradient.
      grad_gamma[a] = {};
      for (const auto& [s_face, _] : mesh[domain_, a]) {
        grad_gamma[a] += kernel_.flux(s_face, a);
      }

      // Compute gamma based on the containment function and fluxes.
      gamma[a] = containment_.contains(r[a]) ? Num{1} : Num{0};
      if (const auto norm_grad_gamma_a = norm(grad_gamma[a]);
          !is_tiny(norm_grad_gamma_a)) {
        const auto n_a = grad_gamma[a] / norm_grad_gamma_a;
        const auto r_a = r[a] + (Num{2} * gamma[a] - Num{1}) * n_a * pow2(h[a]);
        for (const auto& [s_face, _] : mesh[domain_, a]) {
          gamma[a] -= kernel_.antigrad_flux(s_face, r_a, h[a]);
        }
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute the maximum allowed time step.
  template<particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  auto compute_time_step(ParticleArray& particles,
                         Exchange& x = serial_exchange) const -> Num {
    TIT_PROFILE_SECTION("FluidEquations::compute_time_step()");
    using PV = ParticleView<ParticleArray>;

    return x.all_reduce_min(par::fold(
        particles.fluid(),
        std::numeric_limits<Num>::max(),
        [this](Num dt, PV a) {
          // Acoustic time step.
          const auto dt_acoustic =
              CFL_ * h[a] /
              (eos_.sound_speed_from_density(rho[a]) + norm(v[a]));

          // Viscous time step.
          const auto dt_visc = C_visc_ * pow2(h[a]) * rho[a] / mu_;

          // Force time step.
          const auto dt_force =
              C_force_ * sqrt(h[a] / std::max(norm(dv_dt[a]), g_));

          return std::min({dt, dt_acoustic, dt_visc, dt_force});
        },
        [](Num dt_a, Num dt_b) { return std::min(dt_a, dt_b); }));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Integration steps.
  //

  /// Compute continuity equation right-hand side.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_continuity(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_continuity()");
    using PV = ParticleView<ParticleArray>;

    // Compute sound speed from density.
    par::for_each(particles.all(), [this](PV a) {
      cs[a] = eos_.sound_speed_from_density(rho[a]);
    });

    // Compute density time derivative.
    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      drho_dt[a] = {};
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);
        drho_dt[a] -= rho[s] * dot(v[a, s], grad_gamma_as) / gamma[a];
      }
    });
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);

      // Ferrari artificial density diffusion term (Ferrari et al., 2009).
      const auto cs_ab = std::max(cs[a], cs[b]);
      const auto Psi_ab = cs_ab * rho[a, b] * r[a, b] / norm(r[a, b]);

      drho_dt[a] += m[b] / gamma[a] * dot(v[a, b] + Psi_ab / rho[b], grad_W_ab);
      drho_dt[b] -= m[a] / gamma[b] * dot(v[b, a] + Psi_ab / rho[a], grad_W_ab);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute momentum equation right-hand side.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_momentum(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_momentum()");
    using PV = ParticleView<ParticleArray>;

    // Compute pressure from density.
    par::for_each(particles.all(),
                  [this](PV a) { p[a] = eos_.pressure_from_density(rho[a]); });

    // Compute velocity time derivative.
    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      dv_dt[a] = unit<1>(r[a], -g_);
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);

        const auto P_as = rho[s] * (p[a] / pow2(rho[a]) + p[s] / pow2(rho[s]));

        const auto n_s = normalize(grad_gamma_as);
        const auto t_as = normalize(v[a, s] - dot(v[a, s], n_s) * n_s);
        const auto dr_as = std::max(h[a] / 2, dot(r[a, s], n_s));
        const auto Pi_as =
            2 * mu_ / (rho[a] * dr_as) * dot(v[a, s], t_as) * t_as;

        dv_dt[a] +=
            (P_as * grad_gamma_as - Pi_as * norm(grad_gamma_as)) / gamma[a];
      }
    });
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto grad_W_ab = kernel_.grad(a, b);

      const auto P_ab = p[a] / pow2(rho[a]) + p[b] / pow2(rho[b]);

      const auto Pi_ab =
          2 * mu_ * dot(v[a, b], r[a, b]) / (rho[a] * rho[b] * norm2(r[a, b]));

      dv_dt[a] += m[b] / gamma[a] * (Pi_ab - P_ab) * grad_W_ab;
      dv_dt[b] -= m[a] / gamma[b] * (Pi_ab - P_ab) * grad_W_ab;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Post-integration steps.
  //

  /// Run post-integration mesh refresh and correction steps.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  void post_integrate(ParticleMesh& mesh,
                      ParticleArray& particles,
                      Exchange& x = serial_exchange) const {
    TIT_PROFILE_SECTION("FluidEquations::post_integrate()");

    prepare(mesh, particles, x);
    apply_shifts(mesh, particles, x);
    apply_free_surface_correction(mesh, particles, x);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Apply particle shifts and correct shifted fields.
  ///
  /// This is a post-integration step, thus it requires updated mesh and
  /// boundary extrapolation.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  void apply_shifts(ParticleMesh& mesh,
                    ParticleArray& particles,
                    Exchange& x = serial_exchange) const {
    TIT_PROFILE_SECTION("FluidEquations::apply_shifts()");
    using PV = ParticleView<ParticleArray>;

    // Compute normal vector, normalization matrix, and gradients for each
    // advected field. Owned particles only: the ghost normal vectors are
    // refreshed from their owners below, the remaining quantities of the
    // ghost particles are never read, and the boundary faces adjacent to a
    // ghost particle may refer to vertices beyond the halo.
    par::for_each(particles.owned(), [&mesh, this](PV a) {
      N[a] = {};
      L[a] = {};
      grad_v[a] = {};
      grad_rho[a] = {};
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);

        N[a] -= grad_gamma_as / gamma[a];
        L[a] -= outer(r[s, a], grad_gamma_as) / gamma[a];
        grad_v[a] -= outer(v[s, a], grad_gamma_as) / gamma[a];
        grad_rho[a] -= rho[s, a] * grad_gamma_as / gamma[a];
      }
    });
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto V_a = m[a] / rho[a];
      const auto V_b = m[b] / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      N[a] += V_b / gamma[a] * grad_W_ab;
      N[b] -= V_a / gamma[b] * grad_W_ab;
      L[a] += V_b / gamma[a] * outer(r[b, a], grad_W_ab);
      L[b] -= V_a / gamma[b] * outer(r[a, b], grad_W_ab);
      grad_v[a] += V_b / gamma[a] * outer(v[b, a], grad_W_ab);
      grad_v[b] -= V_a / gamma[b] * outer(v[a, b], grad_W_ab);
      grad_rho[a] += V_b / gamma[a] * rho[b, a] * grad_W_ab;
      grad_rho[b] -= V_a / gamma[b] * rho[a, b] * grad_W_ab;
    });
    par::for_each(particles.owned(), [](PV a) {
      dr[a] = N[a];
      if (const auto fact = lu(transpose(L[a]))) {
        L[a] = fact->inverse();
        N[a] = L[a] * N[a];
        grad_v[a] = grad_v[a] * transpose(L[a]);
        grad_rho[a] = L[a] * grad_rho[a];
      } else {
        L[a] = eye(L[a]);
      }
      N[a] = normalize(N[a]);
    });

    // Ghost normal vectors are incomplete: the pair sums above cover only
    // the locally present neighbors. The visibility test below reads the
    // neighbor normals, so pull the complete values from the owners.
    x.refresh(particles, TypeSet{N});

    // Initialize the free surface flag indicators.
    // - `phi_max = 1` means that the particle is far from the free surface.
    // - Any value in the range `(phi_min, phi_max)` means that the particle is
    //   near the free surface (has at least one neighbor that is on the free
    //   surface).
    // - Value `phi_min` means that the particle is on the free surface. It is
    //   essentially zero, but we use a very small number to avoid spurious
    //   comparisons.
    par::for_each(particles.fixed(), [](PV a) { phi[a] = phi_max_; });
    par::for_each(particles.fluid(), [](PV a) { phi[a] = phi_min_; });
    x.refresh(particles, TypeSet{phi});

    // Classify the particles into free surface and non-free surface.
    //
    // Here we are reading and writing the same field `phi` in the parallel
    // loop. There is no race condition because we read the neighbor to compare
    // it with `phi_min`, and non-free-surface particles are updated in the
    // loop.
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;

      // Skip the particles that are too far away.
      const auto r2_ab = norm2(r[a, b]);
      const auto dist_threshold = avg(kernel_.radius(a), kernel_.radius(b));
      if (r2_ab > pow2(dist_threshold)) return;

      // Perform "visibility" test. The actual test is just an optimized
      // version of `acos(n_{a,b} / sqrt(r_ab)) <= fov`.
      constexpr auto cos_fov = static_cast<Num>(cos(std::numbers::pi / 4));
      const auto fov_threshold = pow2(cos_fov) * r2_ab;
      if (bitwise_equal(phi[a], phi_min_)) {
        const auto n_a = dot(N[a], r[a, b]);
        if (n_a > 0 && pow2(n_a) >= fov_threshold) phi[a] = phi_max_;
      }
      if (bitwise_equal(phi[b], phi_min_)) {
        const auto n_b = dot(N[b], r[a, b]);
        if (n_b < 0 && pow2(n_b) >= fov_threshold) phi[b] = phi_max_;
      }
    });

    // Mark splashes as free surface particle.
    par::for_each(particles.fluid(), [&mesh](PV a) {
      // We shall consider a particle as a splash if it has:
      // - less than 8 neighbors in 2D and
      // - less than 26 neighbors in 3D.
      static constexpr std::size_t neighbor_cutoff =
          particle_dim_v<PV> == 2 ? 8 : 26;
      if (std::ranges::size(mesh[a]) <= neighbor_cutoff) phi[a] = phi_min_;
    });

    // The classification below reads the neighbor indicators, so pull the
    // classified values onto the ghost particles.
    x.refresh(particles, TypeSet{phi});

    // Classify the non-free surface particles into near and far categories.
    //
    // Here we are reading and writing the same field `phi` in the parallel
    // loop. There is no race condition because we update the field only when
    // the particle has `phi_min`, and read the field only to compare it with
    // `phi_min`.
    //
    // A distinct non-zero bit pattern of `phi_min` is essential for
    // correctness. We may read a garbage value while memory is being updated by
    // some other thread, and the chances of a false positive comparison with
    // distinct bits are very small, at least orders of magnitude smaller than
    // if we used zero.
    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      if (!bitwise_equal(phi[a], phi_max_)) return;

      constexpr auto on_fs = [](PV b) {
        return bitwise_equal(phi[b], phi_min_);
      };
      if (std::ranges::any_of(mesh[a], on_fs)) {
        auto fs_neighbors = std::views::filter(mesh[a], on_fs);
        const auto dist_to_a = [a](PV b) { return norm2(r[a, b]); };
        const auto b = *std::ranges::min_element(fs_neighbors, {}, dist_to_a);
        phi[a] *= abs(dot(N[b], r[a, b])) / kernel_.radius(a);
      }
    });

    // Apply the particle shifts and correct fields.
    par::for_each(particles.fluid(), [](PV a) {
      // Here we'll follow Leroy's PhD thesis (2014) and apply shifts only to
      // the far-away particles.
      if (!bitwise_equal(phi[a], phi_max_)) {
        dr[a] = {};
        return;
      }

      dr[a] *= -CFL_ * C_shift_ * pow2(h[a]);
      r[a] += dr[a];
      // Theoretically, the correction below should be applied unconditionally,
      // but applying it near the walls can cause sudden significant changes to
      // the velocity field even for slow laminar flows.
      if (approx_equal_to(gamma[a], Num{1})) v[a] += grad_v[a] * dr[a];
      rho[a] += dot(grad_rho[a], dr[a]);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Apply free-surface density correction.
  ///
  /// This is a post-shifting step: it uses the free-surface marker produced
  /// while shifting. The caller may refresh the mesh and boundary state before
  /// calling this method, but this is usually unnecessary because the
  /// correction is applied only to unshifted particles whose kernel deficit is
  /// not already explained by gamma.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray,
           exchange_for<ParticleArray> Exchange = const SerialExchange>
  void apply_free_surface_correction(ParticleMesh& mesh,
                                     ParticleArray& particles,
                                     Exchange& x = serial_exchange) const {
    TIT_PROFILE_SECTION("FluidEquations::apply_free_surface_correction()");
    using PV = ParticleView<ParticleArray>;

    // The shifts have moved the owned particles and corrected their density.
    x.refresh(particles, TypeSet{r, rho});

    par::for_each(particles.all(), [](PV a) { rho_raw[a] = rho[a]; });

    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      // Correction is not applied to far-away particles.
      if (bitwise_equal(phi[a], phi_max_)) return;

      // Compute Shepard's filter value and direct density estimate.
      Num alpha_a{};
      Num rho_tilde_a{};
      for (const auto b : mesh[a]) {
        const auto W_ab = kernel_(a, b);
        alpha_a += m[b] / rho_raw[b] * W_ab;
        rho_tilde_a += m[b] * W_ab;
      }

      // Skip particles whose kernel deficit is already explained by gamma.
      const auto alpha_ratio = std::min(Num{1}, alpha_a / gamma[a]);
      if (alpha_ratio > Num{0.99}) return;

      const auto beta = exp(-K_free_surface_ * pow2(alpha_ratio - 1));
      const auto correction = beta * gamma[a] + (1 - beta) * alpha_a;
      if (!is_tiny(correction)) rho[a] = rho_tilde_a / correction;
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  static constexpr Num CFL_{0.4};
  static constexpr Num C_force_{0.25};
  static constexpr Num C_visc_{0.125};
  static constexpr Num C_shift_{0.2};
  static constexpr Num phi_max_{1};
  static constexpr Num phi_min_{std::numeric_limits<Num>::min()};
  static constexpr Num K_free_surface_{-log(Num{0.05}) / pow2(Num{0.01})};

  Num g_;
  Num mu_;
  [[no_unique_address]] Domain domain_;
  [[no_unique_address]] Containment containment_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
