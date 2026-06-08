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

#include "tit/core/assert.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/buoyancy.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/turbulence_model.hpp"
#include "tit/sph/viscosity.hpp"
#include "tit/sph/wall_model.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fluid equations with fixed kernel width and continuity equation.
template<class Num,
         equation_of_state<Num> EquationOfState,
         viscosity_model<Num> ViscosityModel,
         turbulence_model<Num> TurbulenceModel,
         buoyancy_model<Num> BuoyancyModel,
         pressure_wall_model<Num> PressureWallModel,
         velocity_wall_model<Num> VelocityWallModel,
         thermal_wall_model<Num> ThermalWallModel,
         kernel Kernel,
         class Domain>
class FluidEquations final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = //
      TypeSet{h, m, gamma, grad_gamma, rho, drho_dt, grad_rho, p, cs} |
      TypeSet{v, dv_dt, grad_v, r, dr, L, N, phi, rho_raw} |
      TypeSet{T, dT_dt, grad_T, mu};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      TypeSet{m, gamma, grad_gamma, rho, drho_dt, grad_rho, p, cs} |
      TypeSet{v, dv_dt, grad_v, r, dr, N, L, phi, rho_raw} |
      TypeSet{T, dT_dt, grad_T, mu};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct the fluid equations.
  ///
  /// @param g                   Gravitational acceleration.
  /// @param k                   Thermal conductivity.
  /// @param c_v                 Specific heat at constant volume.
  /// @param domain              Boundary domain.
  /// @param eos                 Equation of state.
  /// @param viscosity_model     Viscosity model.
  /// @param turbulence_model    Turbulence model.
  /// @param buoyancy_model      Buoyancy model.
  /// @param pressure_wall_model Pressure wall model.
  /// @param velocity_wall_model Velocity wall model.
  /// @param thermal_wall_model  Thermal wall model.
  /// @param kernel              Kernel.
  constexpr explicit FluidEquations(Num g,
                                    Num k,
                                    Num c_v,
                                    const Domain& domain,
                                    EquationOfState eos,
                                    ViscosityModel viscosity_model,
                                    TurbulenceModel turbulence_model,
                                    BuoyancyModel buoyancy_model,
                                    PressureWallModel pressure_wall_model,
                                    VelocityWallModel velocity_wall_model,
                                    ThermalWallModel thermal_wall_model,
                                    Kernel kernel) noexcept
      : g_{g}, k_{k}, c_v_{c_v}, //
        domain_{std::move(domain)}, eos_{std::move(eos)},
        viscosity_model_{std::move(viscosity_model)},
        turbulence_model_{std::move(turbulence_model)},
        buoyancy_model_{std::move(buoyancy_model)}, //
        pressure_wall_model_{std::move(pressure_wall_model)},
        velocity_wall_model_{std::move(velocity_wall_model)},
        thermal_wall_model_{std::move(thermal_wall_model)},
        kernel_{std::move(kernel)} {
    TIT_ASSERT(k_ >= Num{0}, "Thermal conductivity must be non-negative!");
    TIT_ASSERT(c_v_ > Num{0}, "Specific heat capacity must be positive!");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Initialization steps.
  //

  /// Initialize equation-owned particle fields.
  template<particle_array<required_fields> ParticleArray>
  void initialize(ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::initialize()");
    using PV = ParticleView<ParticleArray>;

    initialize_gamma(particles);

    // Compute derived fields.
    par::for_each(particles.all(), [this](PV a) {
      cs[a] = eos_.sound_speed_from_density(rho[a]);
      mu[a] = viscosity_model_.dynamic_viscosity(T[a], Num{});
    });

    // Fixed particles have mass fractions.
    par::for_each(particles.fixed(), [](PV a) { m[a] *= gamma[a]; });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize the boundary renormalization field.
  template<particle_array<required_fields> ParticleArray>
  void initialize_gamma(ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::initialize_gamma()");
    using PV = ParticleView<ParticleArray>;
    using Vec = particle_vec_t<ParticleArray>;

    // Compute gamma at the initial time using Ferrand's algorithm
    // (Ferrand et al., 2012).
    //
    // For each particle, first compute ∇γ. From this, two cases arise:
    //
    // - ∇γ is zero, the particle is far from the boundary.
    //   In this case, we can simply set γ = 1.
    //
    // - ∇γ is non-zero, the particle is near the boundary, and γ < 1.
    //   Define:
    //
    //     n := ∇γ / |∇γ|, ẟ := -2 R n, where R is the kernel radius.
    //
    //   One can virtually translate particle by -ẟ, where it is safe to assume
    //   γ(r - ẟ) = 1. γ(r) can be computed by integrating ODEs with respect to
    //   virtual time τ at τ=1:
    //
    //     dx/dτ = ẟ,       x(τ=0) = r - ẟ,
    //     dγ/dτ = ẟ⋅∇γ(x), γ(τ=0) = 1.
    //
    par::for_each(particles.all(), [this](PV a) {
      const auto grad_gamma_at = [this, a](const Vec& x) {
        Vec grad{};
        for (const auto& s_face : domain_.faces()) {
          grad += kernel_.flux(s_face, x, h[a]);
        }
        return grad;
      };

      grad_gamma[a] = grad_gamma_at(r[a]);
      gamma[a] = Num{1};

      const auto grad_gamma_0_norm = norm(grad_gamma[a]);
      if (is_tiny(grad_gamma_0_norm)) return;

      const auto n = grad_gamma[a] / grad_gamma_0_norm;
      const auto delta = -Num{2} * kernel_.radius(h[a]) * n;
      const auto max_abs_dgamma_dtau = abs(dot(grad_gamma[a], delta));

      auto x = r[a] - delta;
      auto grad_gamma_x = grad_gamma_at(x);
      for (Num tau{}; tau < Num{1};) {
        const auto remaining = Num{1} - tau;
        const auto abs_dgamma_dtau = abs(dot(grad_gamma_x, delta));
        const auto denom = std::max(abs_dgamma_dtau, max_abs_dgamma_dtau);
        auto dtau =
            is_tiny(denom) ? remaining : std::min(remaining, C_gamma_ / denom);
        if (tau + dtau <= tau) dtau = remaining;

        const auto dx = dtau * delta;
        const auto next_x = x + dx;
        const auto next_grad_gamma_x = grad_gamma_at(next_x);

        gamma[a] += dot(grad_gamma_x + next_grad_gamma_x, dx) / 2;

        x = next_x;
        grad_gamma_x = next_grad_gamma_x;

        tau += dtau;
      }

      gamma[a] = std::clamp(gamma[a], Num{0}, Num{1});
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Preparation steps.
  //

  /// Refresh mesh and boundary state before evaluating equations.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void prepare(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::prepare()");

    index(mesh, particles);
    setup_boundary(mesh, particles);
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

    par::for_each(particles.fluid(),
                  [this](PV a) { p[a] = eos_.pressure_from_density(rho[a]); });

    // Reconstruct the state on the wall particles from nearby fluid.
    par::for_each(particles.fixed(), [this, &mesh](PV e) {
      const auto n_e = normalize(grad_gamma[e]);
      const auto g_vec = unit<1>(r[e], -g_);
      const auto neighbors = mesh[e];

      const auto weight = [this, e](PV b) {
        if (!b.is_fluid()) return Num{};
        return m[b] / rho[b] * kernel_(r[b] - r[e], h[e]);
      };
      const auto normal_distance = [e, n_e](PV b) {
        return dot(r[b] - r[e], n_e);
      };
      const auto tangential_offset = [e, n_e](PV b) {
        const auto r_be = r[b] - r[e];
        return r_be - dot(r_be, n_e) * n_e;
      };

      p[e] = pressure_wall_model_.pressure(
          neighbors,
          normal_distance,
          weight,
          [](PV b) { return p[b]; },
          [](PV b) { return rho[b]; },
          tangential_offset,
          n_e,
          g_vec,
          Num{});
      rho[e] = eos_.density_from_pressure(p[e]);
      cs[e] = eos_.sound_speed_from_density(rho[e]);
      v[e] = velocity_wall_model_.velocity(
          neighbors,
          normal_distance,
          weight,
          [](PV b) { return v[b]; },
          n_e);
      T[e] = thermal_wall_model_.temperature(
          neighbors,
          normal_distance,
          weight,
          [](PV b) { return T[b]; },
          k_,
          T[e]);
      mu[e] = viscosity_model_.dynamic_viscosity(T[e], Num{});
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute the maximum allowed time step.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  auto compute_time_step(ParticleMesh& mesh, ParticleArray& particles) const
      -> Num {
    TIT_PROFILE_SECTION("FluidEquations::compute_time_step()");
    using PV = ParticleView<ParticleArray>;

    return par::fold(
        particles.fluid(),
        std::numeric_limits<Num>::max(),
        [&mesh, this](Num dt, PV a) {
          // Acoustic time step.
          const auto dt_acoustic = CFL_ * h[a] / (cs[a] + norm(v[a]));

          // Viscous time step.
          const auto dt_visc =
              C_diff_ * pow2(h[a]) * rho[a] / std::max(mu[a], tiny_v<Num>);

          // Thermal diffusion time step.
          const auto dt_thermal =
              C_diff_ * pow2(h[a]) * rho[a] * c_v_ / std::max(k_, tiny_v<Num>);

          // Force time step.
          const auto dt_force =
              C_force_ * sqrt(h[a] / std::max(norm(dv_dt[a]), g_));

          // Gamma time step.
          auto dt_gamma = std::numeric_limits<Num>::max();
          for (const auto& [s_face, s] : mesh[domain_, a]) {
            const auto grad_gamma_as = kernel_.flux(s_face, a);
            dt_gamma = std::min(
                dt_gamma,
                C_gamma_ / abs(dot(grad_gamma_as, v[a, s]) + tiny_v<Num>));
          }

          return std::min({
              dt,
              dt_acoustic,
              dt_visc,
              dt_thermal,
              dt_force,
              dt_gamma,
          });
        },
        [](Num dt_a, Num dt_b) { return std::min(dt_a, dt_b); });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Integration steps.
  //

  /// Compute gradient of gamma.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void compute_gamma_gradient(ParticleMesh& mesh,
                              ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::compute_gamma_gradient()");
    using PV = ParticleView<ParticleArray>;

    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      grad_gamma[a] = {};
      for (const auto& [s_face, _] : mesh[domain_, a]) {
        grad_gamma[a] += kernel_.flux(s_face, a);
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

    // Compute velocity gradient.
    par::for_each(particles.all(), [&mesh, this](PV a) {
      L[a] = {};
      grad_v[a] = {};
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);
        L[a] -= outer(r[s, a], grad_gamma_as) / gamma[a];
        grad_v[a] -= outer(v[s, a], grad_gamma_as) / gamma[a];
      }
    });
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto V_a = m[a] / rho[a];
      const auto V_b = m[b] / rho[b];
      const auto grad_W_ab = kernel_.grad(a, b);

      L[a] += V_b / gamma[a] * outer(r[b, a], grad_W_ab);
      L[b] -= V_a / gamma[b] * outer(r[a, b], grad_W_ab);
      grad_v[a] += V_b / gamma[a] * outer(v[b, a], grad_W_ab);
      grad_v[b] -= V_a / gamma[b] * outer(v[a, b], grad_W_ab);
    });

    // Finalize velocity gradients and compute thermodynamic properties.
    par::for_each(particles.all(), [this](PV a) {
      if (const auto fact = lu(transpose(L[a]))) {
        L[a] = fact->inverse();
        grad_v[a] = grad_v[a] * transpose(L[a]);
      } else {
        L[a] = eye(L[a]);
      }
      const auto S_a = (grad_v[a] + transpose(grad_v[a])) / 2;
      const auto S_mag_a = sqrt(2 * tr(S_a * transpose(S_a)));
      p[a] = eos_.pressure_from_density(rho[a]);
      mu[a] = viscosity_model_.dynamic_viscosity(T[a], S_mag_a) +
              turbulence_model_.eddy_viscosity(rho[a], S_mag_a);
    });

    // Compute velocity and temperature time derivatives.
    par::for_each(particles.fluid(), [&mesh, this](PV a) {
      dv_dt[a] = unit<1>(r[a], -g_) * (1 + buoyancy_model_.coeff(T[a]));
      dT_dt[a] = {};
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);

        const auto n_s = s_face.normal();
        const auto v_tangent_as = v[a, s] - dot(v[a, s], n_s) * n_s;
        const auto dr_as = std::max(h[a] / 2, dot(r[a, s], n_s));

        const auto F_pres_as =
            rho[s] * (p[a] / pow2(rho[a]) + p[s] / pow2(rho[s]));
        const auto F_visc_as =
            2 / rho[a] *
            velocity_wall_model_.shear_stress(rho[a],
                                              (mu[a] + mu[s]) / 2,
                                              dr_as,
                                              v_tangent_as);

        dv_dt[a] +=
            (F_pres_as * grad_gamma_as - F_visc_as * norm(grad_gamma_as)) /
            gamma[a];

        const auto Q_visc_as =
            (2 / rho[a] * c_v_) * dot(F_visc_as, v_tangent_as);
        const auto Q_diff_as =
            (2 / rho[a] * c_v_) *
            thermal_wall_model_.heat_flux(k_, T[a], T[s], r[a, s], n_s);

        dT_dt[a] += (Q_visc_as - Q_diff_as) * norm(grad_gamma_as) / gamma[a];
      }
    });
    par::block_for_each(mesh.block_pairs(particles), [this](auto ab) {
      const auto [a, b] = ab;
      const auto r2_ab = norm2(r[a, b]) + pow2(Num{0.01} * h.avg(a, b));
      const auto grad_W_ab = kernel_.grad(a, b);

      const auto F_pres_ab = p[a] / pow2(rho[a]) + p[b] / pow2(rho[b]);
      const auto F_visc_ab =
          2 * mu.avg(a, b) * dot(v[a, b], r[a, b]) / (rho[a] * rho[b] * r2_ab);

      dv_dt[a] += m[b] / gamma[a] * (F_visc_ab - F_pres_ab) * grad_W_ab;
      dv_dt[b] -= m[a] / gamma[b] * (F_visc_ab - F_pres_ab) * grad_W_ab;

      const auto Q_visc_ab = F_visc_ab * v[a, b] / (2 * c_v_);
      const auto Q_diff_ab =
          2 * k_ * T[a, b] * r[a, b] / (c_v_ * rho[a] * rho[b] * r2_ab);

      dT_dt[a] += m[b] / gamma[a] * dot(Q_diff_ab + Q_visc_ab, grad_W_ab);
      dT_dt[b] -= m[a] / gamma[b] * dot(Q_diff_ab - Q_visc_ab, grad_W_ab);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Post-integration steps.
  //

  /// Run post-integration mesh refresh and correction steps.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void post_integrate(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::post_integrate()");

    prepare(mesh, particles);
    apply_shifts(mesh, particles);
    apply_free_surface_correction(mesh, particles);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Apply particle shifts and correct shifted fields.
  ///
  /// This is a post-integration step, thus it requires updated mesh and
  /// boundary extrapolation.
  template<particle_mesh ParticleMesh,
           particle_array<required_fields> ParticleArray>
  void apply_shifts(ParticleMesh& mesh, ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::apply_shifts()");
    using PV = ParticleView<ParticleArray>;

    // Compute normal vector, normalization matrix, and gradients for each
    // advected field.
    par::for_each(particles.all(), [&mesh, this](PV a) {
      N[a] = {};
      L[a] = {};
      grad_v[a] = {};
      grad_T[a] = {};
      grad_rho[a] = {};
      grad_gamma[a] = {};
      for (const auto& [s_face, s] : mesh[domain_, a]) {
        const auto grad_gamma_as = kernel_.flux(s_face, a);

        N[a] -= grad_gamma_as / gamma[a];
        L[a] -= outer(r[s, a], grad_gamma_as) / gamma[a];
        grad_v[a] -= outer(v[s, a], grad_gamma_as) / gamma[a];
        grad_T[a] -= T[s, a] * grad_gamma_as / gamma[a];
        grad_rho[a] -= rho[s, a] * grad_gamma_as / gamma[a];
        grad_gamma[a] += grad_gamma_as;
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
      grad_T[a] += V_b / gamma[a] * T[b, a] * grad_W_ab;
      grad_T[b] -= V_a / gamma[b] * T[a, b] * grad_W_ab;
      grad_rho[a] += V_b / gamma[a] * rho[b, a] * grad_W_ab;
      grad_rho[b] -= V_a / gamma[b] * rho[a, b] * grad_W_ab;
    });
    par::for_each(particles.all(), [](PV a) {
      dr[a] = N[a];
      if (const auto fact = lu(transpose(L[a]))) {
        L[a] = fact->inverse();
        N[a] = L[a] * N[a];
        grad_v[a] = grad_v[a] * transpose(L[a]);
        grad_T[a] = L[a] * grad_T[a];
        grad_rho[a] = L[a] * grad_rho[a];
      } else {
        L[a] = eye(L[a]);
      }
      N[a] = normalize(N[a]);
    });

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
      T[a] += dot(grad_T[a], dr[a]);
      rho[a] += dot(grad_rho[a], dr[a]);
      gamma[a] += dot(grad_gamma[a], dr[a]);
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
           particle_array<required_fields> ParticleArray>
  void apply_free_surface_correction(ParticleMesh& mesh,
                                     ParticleArray& particles) const {
    TIT_PROFILE_SECTION("FluidEquations::apply_free_surface_correction()");
    using PV = ParticleView<ParticleArray>;

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
  static constexpr Num C_diff_{0.125};
  static constexpr Num C_gamma_{0.004};
  static constexpr Num C_shift_{0.2};
  static constexpr Num phi_max_{1};
  static constexpr Num phi_min_{std::numeric_limits<Num>::min()};
  static constexpr Num K_free_surface_{-log(Num{0.05}) / pow2(Num{0.01})};

  Num g_;
  Num k_;
  Num c_v_;
  [[no_unique_address]] Domain domain_;
  [[no_unique_address]] EquationOfState eos_;
  [[no_unique_address]] ViscosityModel viscosity_model_;
  [[no_unique_address]] TurbulenceModel turbulence_model_;
  [[no_unique_address]] BuoyancyModel buoyancy_model_;
  [[no_unique_address]] PressureWallModel pressure_wall_model_;
  [[no_unique_address]] VelocityWallModel velocity_wall_model_;
  [[no_unique_address]] ThermalWallModel thermal_wall_model_;
  [[no_unique_address]] Kernel kernel_;

}; // class FluidEquations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
