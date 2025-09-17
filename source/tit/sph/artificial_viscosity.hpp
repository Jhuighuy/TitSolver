/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No artificial viscosity, in case physical viscosity is strong enough.
class NoArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Continuity equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return zero(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return zero(rho[a, b]);
  }

}; // class NoArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// α-β artificial viscosity (Monaghan, 1992).
template<class Num>
class AlphaBetaArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho, h, r, v, cs};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param alpha Linear viscosity coefficient.
  /// @param beta   Quadratic viscosity coefficient. Typically two times greater
  ///               than linear coefficient for compressible flows and zero for
  ///               weakly-compressible or incompressible flows.
  constexpr explicit AlphaBetaArtificialViscosity(Num alpha = 1.0,
                                                  Num beta = 2.0) noexcept
      : alpha_{alpha}, beta_{beta} {
    TIT_ASSERT(alpha_ > 0.0, "Linear coefficient must be positive.");
    TIT_ASSERT(beta_ >= 0.0, "Quadratic coefficient must be non-negative.");
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return zero(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    if (dot(v[a, b], r[a, b]) >= 0.0) return 0.0;
    const auto h_ab = h.avg(a, b);
    const auto rho_ab = rho.avg(a, b);
    const auto cs_ab = cs.avg(a, b);
    const auto mu_ab = h_ab * dot(v[a, b], r[a, b]) / norm2(r[a, b]);
    return (alpha_ * cs_ab - beta_ * mu_ab) * mu_ab / rho_ab;
  }

private:

  Num alpha_;
  Num beta_;

}; // class AlphaBetaArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// ξ-SPH artificial viscosity (Molteni, Colagrossi, 2009).
/// Weakly-compressible SPH formulation is assumed.
template<class Num>
class MolteniColagrossiArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho, grad_rho, h, r, v};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param xi    Density diffusion coefficient. Typically 0.1.
  constexpr explicit MolteniColagrossiArtificialViscosity(Num cs_0,
                                                          Num rho_0,
                                                          Num alpha = 0.02,
                                                          Num xi = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, xi_{xi} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(xi_ > 0.0, "Density coefficient must be positive.");
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto D_ab = rho[a, b];
    const auto Xi_ab = xi_ * cs_0_ * h_ab;
    return 2 * Xi_ab * D_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto Alpha_ab = alpha_ * cs_0_ * rho_0_ * h_ab;
    return Alpha_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num alpha_;
  Num xi_;

}; // class MolteniColagrossiArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// δ-SPH artificial viscosity (Marrone, 2011).
/// Weakly-compressible SPH formulation is assumed.
template<class Num>
class DeltaSPHArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho, grad_rho, h, r, L, v};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param delta Density diffusion coefficient. Typically 0.1.
  constexpr explicit DeltaSPHArtificialViscosity(Num cs_0,
                                                 Num rho_0,
                                                 Num alpha = 0.02,
                                                 Num delta = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, delta_{delta} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive!");
    TIT_ASSERT(delta_ > 0.0, "Density coefficient must be positive!");
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    // Here we assume that density gradients are renormalized because
    // kernel gradient renormalization filter (`L`) was requested.
    const auto D_ab = rho[a, b] - dot(grad_rho.avg(a, b), r[a, b]);
    const auto Delta_ab = delta_ * cs_0_ * h_ab;
    return 2 * Delta_ab * D_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto Alpha_ab = alpha_ * cs_0_ * rho_0_ * h_ab;
    return Alpha_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num alpha_;
  Num delta_;

}; // class DeltaSPHArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Artificial viscosity type.
template<class AV>
concept artificial_viscosity = //
    std::same_as<AV, NoArtificialViscosity> ||
    specialization_of<AV, AlphaBetaArtificialViscosity> ||
    specialization_of<AV, MolteniColagrossiArtificialViscosity> ||
    specialization_of<AV, DeltaSPHArtificialViscosity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
