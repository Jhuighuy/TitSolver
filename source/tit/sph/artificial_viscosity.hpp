/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// ξ-SPH artificial viscosity (Molteni, Colagrossi, 2009).
/// Weakly-compressible SPH formulation is assumed.
template<class Num>
class MolteniColagrossiArtificialViscosity final {
public:

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

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{h};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho, r, v};
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto D_ab = rho[a, b];
    const auto Xi_ab = xi_ * cs_0_ * h_ab;
    return 2 * Xi_ab * D_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num> PV>
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

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{h};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho, grad_rho, r, L, v};
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num> PV>
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
  template<particle_view_n<Num> PV>
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

} // namespace tit::sph
