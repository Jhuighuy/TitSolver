/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
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
template<class Num>
class NoArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

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
  /// @param beta  Quadratic viscosity coefficient. Typically two times greater
  ///              than linear coefficient for compressible flows and zero for
  ///              weakly-compressible or incompressible flows.
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

/// δ-SPH artificial viscosity (Marrone, 2011).
/// Weakly-compressible SPH formulation is assumed.
template<class Num>
class DeltaSPHArtificialViscosity final {
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
    const auto D_ab = rho[a, b] - dot(grad_rho.avg(a, b), r[a, b]);
    const auto delta_ab = delta_ * cs_0_ * h_ab;
    return 2 * delta_ab * D_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto alpha_ab = alpha_ * cs_0_ * rho_0_ * h_ab;
    return alpha_ab * dot(r[a, b], v[a, b]) /
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
template<class AV, class Num>
concept artificial_viscosity = //
    std::same_as<AV, NoArtificialViscosity<Num>> ||
    std::same_as<AV, AlphaBetaArtificialViscosity<Num>> ||
    std::same_as<AV, DeltaSPHArtificialViscosity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
