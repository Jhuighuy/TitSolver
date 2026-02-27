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
  static constexpr TypeSet fields{/*empty*/};

  /// Continuity equation diffusive term.
  template<particle_view_n<Num> PV>
  constexpr auto continuity_equation_term(PV /*a*/, PV /*b*/) const noexcept
      -> particle_vec_t<PV> {
    return {};
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num> PV>
  constexpr auto momentum_equation_term(PV /*a*/, PV /*b*/) const noexcept
      -> particle_num_t<PV> {
    return {};
  }

}; // class NoArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// α-β artificial viscosity formulation (Monaghan, 1992).
template<class Num>
class AlphaBetaArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, r, v, cs};

  /// Construct artificial viscosity formulation.
  ///
  /// @param h     Particle width.
  /// @param alpha Linear viscosity coefficient.
  /// @param beta  Quadratic viscosity coefficient. Typically two times greater
  ///              than linear coefficient for compressible flows and zero for
  ///              weakly-compressible or incompressible flows.
  constexpr explicit AlphaBetaArtificialViscosity(Num h,
                                                  Num alpha = 1.0,
                                                  Num beta = 2.0) noexcept
      : h_{h}, alpha_{alpha}, beta_{beta} {
    TIT_ASSERT(h_ > 0.0, "Particle width must be positive.");
    TIT_ASSERT(alpha_ > 0.0, "Linear coefficient must be positive.");
    TIT_ASSERT(beta_ >= 0.0, "Quadratic coefficient must be non-negative.");
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num> PV>
  constexpr auto continuity_equation_term(PV /*a*/, PV /*b*/) const noexcept
      -> particle_vec_t<PV> {
    return {};
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, rho, cs, r, v> PV>
  constexpr auto momentum_equation_term(PV a, PV b) const noexcept
      -> particle_num_t<PV> {
    if (dot(v[a, b], r[a, b]) >= 0.0) return 0.0;
    const auto rho_ab = rho.avg(a, b);
    const auto cs_ab = cs.avg(a, b);
    const auto mu_ab = h_ * dot(v[a, b], r[a, b]) / norm2(r[a, b]);
    return (alpha_ * cs_ab - beta_ * mu_ab) * mu_ab / rho_ab;
  }

private:

  Num h_;
  Num alpha_;
  Num beta_;

}; // class AlphaBetaArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// ξ-SPH artificial viscosity formulation for weakly-compressible SPH
/// (Molteni, Colagrossi, 2009).
template<class Num>
class MolteniColagrossiArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, grad_rho, r, v};

  /// Construct artificial viscosity formulation.
  ///
  /// @param h     Particle width.
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param xi    Density diffusion coefficient. Typically 0.1.
  constexpr explicit MolteniColagrossiArtificialViscosity(
      Num h,
      Num cs_0,
      Num rho_0,
      Num alpha = 0.02,
      Num xi = 0.1) noexcept {
    TIT_ASSERT(h > 0.0, "Particle width must be positive.");
    TIT_ASSERT(cs_0 > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0 > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(alpha > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(xi > 0.0, "Density coefficient must be positive.");
    Xi_ = 2 * xi * cs_0 * h;
    Alpha_ = alpha * cs_0 * rho_0 * h;
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num, rho, r> PV>
  constexpr auto continuity_equation_term(PV a, PV b) const noexcept
      -> particle_vec_t<PV> {
    return Xi_ * rho[a, b] * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, rho, r, v> PV>
  constexpr auto momentum_equation_term(PV a, PV b) const noexcept
      -> particle_num_t<PV> {
    return Alpha_ * dot(r[a, b], v[a, b]) / (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num Xi_;
  Num Alpha_;

}; // class MolteniColagrossiArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// δ-SPH artificial viscosity formulation for weakly-compressible SPH
/// (Marrone, 2011).
template<class Num>
class DeltaSPHArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, grad_rho, r, L, v};

  /// Construct artificial viscosity formulation.
  ///
  /// @param h     Particle width.
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param delta Density diffusion coefficient. Typically 0.1.
  constexpr explicit DeltaSPHArtificialViscosity(Num h,
                                                 Num cs_0,
                                                 Num rho_0,
                                                 Num alpha = 0.02,
                                                 Num delta = 0.1) noexcept {
    TIT_ASSERT(h > 0.0, "Particle width must be positive.");
    TIT_ASSERT(cs_0 > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0 > 0.0, "Reference density speed must be positive!");
    TIT_ASSERT(alpha > 0.0, "Velocity coefficient must be positive!");
    TIT_ASSERT(delta > 0.0, "Density coefficient must be positive!");
    Delta_ = 2 * delta * cs_0 * h;
    Alpha_ = alpha * cs_0 * rho_0 * h;
  }

  /// Continuity equation diffusive term.
  template<particle_view_n<Num, rho, grad_rho, r> PV>
  constexpr auto continuity_equation_term(PV a, PV b) const noexcept
      -> particle_vec_t<PV> {
    const auto rho_ab = rho[a, b] - dot(grad_rho.avg(a, b), r[a, b]);
    return Delta_ * rho_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view_n<Num, rho, r, v> PV>
  constexpr auto momentum_equation_term(PV a, PV b) const noexcept
      -> particle_num_t<PV> {
    return Alpha_ * dot(r[a, b], v[a, b]) / (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num Delta_;
  Num Alpha_;

}; // class DeltaSPHArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Artificial viscosity type.
template<class AV, class Num>
concept artificial_viscosity =
    std::same_as<AV, NoArtificialViscosity<Num>> ||
    std::same_as<AV, AlphaBetaArtificialViscosity<Num>> ||
    std::same_as<AV, MolteniColagrossiArtificialViscosity<Num>> ||
    std::same_as<AV, DeltaSPHArtificialViscosity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
