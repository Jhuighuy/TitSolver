/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No artificial viscosity, in case physical viscosity is strong enough.
class NoArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

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
class AlphaBetaArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, h, r, v, cs};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param alpha Linear viscosity coefficient.
  /// @param beta   Quadratic viscosity coefficient. Typically two times greater
  ///               than linear coefficient for compressible flows and zero for
  ///               weakly-compressible or incompressible flows.
  constexpr explicit AlphaBetaArtificialViscosity(real_t alpha = 1.0,
                                                  real_t beta = 2.0) noexcept
      : alpha_{alpha}, beta_{beta} {
    TIT_ASSERT(alpha_ > 0.0, "Linear coefficient must be positive.");
    TIT_ASSERT(beta_ >= 0.0, "Quadratic coefficient must be non-negative.");
  }

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
    if (dot(v[a, b], r[a, b]) >= 0.0) return 0.0;
    const auto h_ab = h.avg(a, b);
    const auto rho_ab = rho.avg(a, b);
    const auto cs_ab = cs.avg(a, b);
    const auto mu_ab = h_ab * dot(v[a, b], r[a, b]) / norm2(r[a, b]);
    return (alpha_ * cs_ab - beta_ * mu_ab) * mu_ab / rho_ab;
  }

private:

  real_t alpha_;
  real_t beta_;

}; // class AlphaBetaArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Artificial viscosity with Balsara switch (Balsara, 1995).
template<class BaseArtificialViscosity = AlphaBetaArtificialViscosity>
class BalsaraArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      BaseArtificialViscosity::required_fields |
      meta::Set{h, cs, div_v, curl_v};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      BaseArtificialViscosity::modified_fields;

  /// Construct artificial viscosity.
  ///
  /// @param base Base artificial viscosity.
  constexpr explicit BalsaraArtificialViscosity(
      BaseArtificialViscosity base) noexcept
      : base_{std::move(base)} {}

  /// Continuity equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return base_.density_term(a, b);
  }

  /// Momentum equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    auto Pi_ab = base_.velocity_term(a, b);
    if (is_tiny(Pi_ab)) return Pi_ab;
    const auto f = [](PV c) {
      return abs(div_v[c]) /
             (abs(div_v[c]) + norm(curl_v[c]) + 0.0001 * cs[c] / h[c]);
    };
    const auto f_ab = avg(f(a), f(b));
    Pi_ab *= f_ab;
    return Pi_ab;
  }

private:

  [[no_unique_address]] BaseArtificialViscosity base_;

}; // class BalsaraArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Artificial viscosity with Rosswog switch (Rosswog, 2000).
template<class BaseArtificialViscosity = BalsaraArtificialViscosity<>>
class RosswogArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, alpha, dalpha_dt} |
      BaseArtificialViscosity::required_fields;

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      BaseArtificialViscosity::modified_fields;

  /// Construct artificial viscosity scheme.
  ///
  /// @param base      Base artificial viscosity.
  /// @param alpha_min Minimal value of the switch coefficient.
  /// @param alpha_max Maximal value of the switch coefficient.
  /// @param sigma     Decay time inverse scale factor.
  constexpr explicit RosswogArtificialViscosity(BaseArtificialViscosity base,
                                                real_t alpha_min = 0.1,
                                                real_t alpha_max = 2.0,
                                                real_t sigma = 0.1) noexcept
      : base_{std::move(base)}, //
        alpha_min_{alpha_min}, alpha_max_{alpha_max}, sigma_{sigma} {
    TIT_ASSERT(alpha_min_ > 0.0, "Switch minimal value must be positive.");
    TIT_ASSERT(alpha_max_ > alpha_min_,
               "Switch maximal value must be "
               "greater than minimal.");
    TIT_ASSERT(sigma_ > 0.0,
               "Switch decay time inverse scale factor "
               "must be positive.");
  }

  /// Continuity equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return base_.density_term(a, b);
  }

  /// Momentum equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    auto Pi_ab = base_.velocity_term(a, b);
    if (is_tiny(Pi_ab)) return Pi_ab;
    const auto alpha_ab = alpha.avg(a, b);
    Pi_ab *= alpha_ab;
    return Pi_ab;
  }

  /// Switch equation source term.
  template<particle_view<required_fields> PV>
  constexpr auto switch_source(PV a) const noexcept {
    const auto S_a = std::max(-div_v[a], decltype(div_v[a]){0.0});
    const auto tau_a = h[a] / (sigma_ * cs[a]);
    return (alpha_max_ - alpha[a]) * S_a - //
           (alpha[a] - alpha_min_) / tau_a;
  }

private:

  [[no_unique_address]] BaseArtificialViscosity base_;
  real_t alpha_min_;
  real_t alpha_max_;
  real_t sigma_;

}; // class RosswogArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// ξ-SPH artificial viscosity (Molteni, Colagrossi, 2009).
/// Weakly-compressible SPH formulation is assumed.
class MolteniColagrossiArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, grad_rho, h, r, v};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param xi    Density diffusion coefficient. Typically 0.1.
  constexpr explicit MolteniColagrossiArtificialViscosity(
      real_t cs_0,
      real_t rho_0,
      real_t alpha = 0.02,
      real_t xi = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, xi_{xi} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(xi_ > 0.0, "Density coefficient must be positive.");
  }

  /// Continuity equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto D_ab = rho[a, b];
    const auto Xi_ab = xi_ * cs_0_ * h_ab;
    return 2 * Xi_ab * D_ab * r[a, b] / norm2(r[a, b]);
  }

  /// Momentum equation diffusive term.
  template<particle_view<required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto Alpha_ab = alpha_ * cs_0_ * rho_0_ * h_ab;
    return Alpha_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  real_t cs_0_;
  real_t rho_0_;
  real_t alpha_;
  real_t xi_;

}; // class MolteniColagrossiArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// δ-SPH artificial viscosity (Marrone, 2011).
/// Weakly-compressible SPH formulation is assumed.
class DeltaSPHArtificialViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, grad_rho, h, r, L, v};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct artificial viscosity scheme.
  ///
  /// @param cs_0  Reference sound speed, as defined for equation of state.
  /// @param rho_0 Reference density, as defined for equation of state.
  /// @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
  /// @param delta Density diffusion coefficient. Typically 0.1.
  constexpr explicit DeltaSPHArtificialViscosity(real_t cs_0,
                                                 real_t rho_0,
                                                 real_t alpha = 0.02,
                                                 real_t delta = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, delta_{delta} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive!");
    TIT_ASSERT(delta_ > 0.0, "Density coefficient must be positive!");
  }

  /// Continuity equation diffusive term.
  template<particle_view<required_fields> PV>
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
  template<particle_view<required_fields> PV>
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto h_ab = h.avg(a, b);
    const auto Alpha_ab = alpha_ * cs_0_ * rho_0_ * h_ab;
    return Alpha_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  real_t cs_0_;
  real_t rho_0_;
  real_t alpha_;
  real_t delta_;

}; // class DeltaSPHArtificialViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Artificial viscosity type.
template<class AV>
concept artificial_viscosity = //
    std::same_as<AV, NoArtificialViscosity> ||
    std::same_as<AV, AlphaBetaArtificialViscosity> ||
    specialization_of<AV, BalsaraArtificialViscosity> ||
    specialization_of<AV, RosswogArtificialViscosity> ||
    std::same_as<AV, MolteniColagrossiArtificialViscosity> ||
    std::same_as<AV, DeltaSPHArtificialViscosity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
