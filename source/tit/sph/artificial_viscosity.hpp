/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** No artificial viscosity (for the braves).
\******************************************************************************/
class NoArtificialViscosity {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{};

  /** Compute continuity equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    constexpr auto Psi_ab = decltype(auto(v[a]))(0.0);
    return Psi_ab;
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    constexpr auto Pi_ab = 0.0;
    return Pi_ab;
  }

}; // class NoArtificialViscosity

/** Artificial viscosity type. */
template<class ArtificialViscosity>
concept artificial_viscosity =
    std::movable<ArtificialViscosity> &&
    std::derived_from<ArtificialViscosity, NoArtificialViscosity>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** α-β artificial viscosity (Monaghan, 1992).
\******************************************************************************/
class AlphaBetaArtificialViscosity : public NoArtificialViscosity {
private:

  real_t alpha_, beta_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, h, r, v, cs};

  /** Construct artificial viscosity scheme.
   ** @param alpha Linear viscosity coefficient.
   ** @param beta Quadratic viscosity coefficient. Typically two times greater
   **             than linear coefficient for compressible flows and
   **             zero for weakly-compressible or incompressable flows. */
  constexpr explicit AlphaBetaArtificialViscosity( //
      real_t alpha = 1.0, real_t beta = 2.0) noexcept
      : alpha_{alpha}, beta_{beta} {
    TIT_ASSERT(alpha_ > 0.0, "Linear coefficient must be positive.");
    TIT_ASSERT(beta_ >= 0.0, "Quadratic coefficient must be non-negative.");
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    if (dot(v[a, b], r[a, b]) >= 0.0) return 0.0;
    auto const h_ab = h.avg(a, b);
    auto const rho_ab = rho.avg(a, b);
    auto const cs_ab = cs.avg(a, b);
    auto const mu_ab = h_ab * dot(v[a, b], r[a, b]) / norm2(r[a, b]);
    auto const Pi_ab = (alpha_ * cs_ab - beta_ * mu_ab) * mu_ab / rho_ab;
    return Pi_ab;
  }

}; // class AlphaBetaArtificialViscosity

/******************************************************************************\
 ** Artificial viscosity with Balsara switch (Balsara, 1995).
\******************************************************************************/
template<artificial_viscosity BaseArtificialViscosity =
             AlphaBetaArtificialViscosity>
class BalsaraArtificialViscosity : public BaseArtificialViscosity {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, curl_v} |
      BaseArtificialViscosity::required_fields;

  /** Construct artificial viscosity.
   ** @param base Base artificial viscosity. */
  constexpr explicit BalsaraArtificialViscosity(
      BaseArtificialViscosity base = {}) noexcept
      : BaseArtificialViscosity{std::move(base)} {}

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto Pi_ab = BaseArtificialViscosity::velocity_term(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    auto const f = [](PV c) {
      return abs(div_v[c]) /
             (abs(div_v[c]) + norm(curl_v[c]) + 0.0001 * cs[c] / h[c]);
    };
    auto const f_ab = avg(f(a), f(b));
    Pi_ab *= f_ab;
    return Pi_ab;
  }

}; // class BalsaraArtificialViscosity

/******************************************************************************\
 ** Artificial viscosity with Rosswog switch (Rosswog, 2000).
\******************************************************************************/
template<artificial_viscosity BaseArtificialViscosity =
             BalsaraArtificialViscosity<>>
class RosswogArtificialViscosity : public BaseArtificialViscosity {
private:

  real_t alpha_min_, alpha_max_;
  real_t sigma_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, alpha, dalpha_dt} |
      BaseArtificialViscosity::required_fields;

  /** Construct artificial viscosity scheme.
   ** @param base Base artificial viscosity.
   ** @param alpha_min Minimal value of the switch coefficient.
   ** @param alpha_max Maximal value of the switch coefficient.
   ** @param sigma Decay time inverse scale factor. */
  constexpr explicit RosswogArtificialViscosity(
      BaseArtificialViscosity base = {}, //
      real_t alpha_min = 0.1, real_t alpha_max = 2.0,
      real_t sigma = 0.1) noexcept
      : BaseArtificialViscosity{std::move(base)}, //
        alpha_min_{alpha_min}, alpha_max_{alpha_max}, sigma_{sigma} {
    TIT_ASSERT(alpha_min_ > 0.0, "Switch minimal value must be positive.");
    TIT_ASSERT(alpha_max_ > alpha_min_, "Switch maximal value must be "
                                        "greater than minimal.");
    TIT_ASSERT(sigma_ > 0.0, "Switch decay time inverse scale factor "
                             "must be positive.");
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto Pi_ab = BaseArtificialViscosity::velocity_term(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    auto const alpha_ab = alpha.avg(a, b);
    Pi_ab *= alpha_ab;
    return Pi_ab;
  }

  /** Compute switch temporal derivative. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_switch_deriv(PV a) const {
    auto const S_a = plus(-div_v[a]);
    auto const tau_a = h[a] / (sigma_ * cs[a]);
    dalpha_dt[a] = (alpha_max_ - alpha[a]) * S_a - //
                   (alpha[a] - alpha_min_) / tau_a;
  }

}; // class RosswogArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** ξ-SPH artificial viscosity (Molteni, Colagrossi, 2009).
 ** Weakly-compressible SPH formulation is assumed.
\******************************************************************************/
class MolteniColagrossiArtificialViscosity : public NoArtificialViscosity {
private:

  real_t cs_0_, rho_0_;
  real_t alpha_, xi_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, grad_rho, h, r, v};

  /** Construct artificial viscosity scheme.
   ** @param cs_0 Reference sound speed, as defined for equation of state.
   ** @param rho_0 Reference density, as defined for equation of state.
   ** @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
   ** @param xi Density diffusion coefficient. Typically 0.1. */
  constexpr MolteniColagrossiArtificialViscosity( //
      real_t cs_0, real_t rho_0, real_t alpha = 0.05, real_t xi = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, xi_{xi} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(xi_ > 0.0, "Density coefficient must be positive.");
  }

  /** Compute continuity equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto const h_ab = h.avg(a, b);
    auto const D_ab = 2 * rho[a, b];
    auto const Psi_ab = xi_ * h_ab * cs_0_ * D_ab * r[a, b] / norm2(r[a, b]);
    return Psi_ab;
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto const h_ab = h.avg(a, b);
    auto const Pi_ab = alpha_ * h_ab * cs_0_ * rho_0_ / (rho[a] * rho[b]) *
                       dot(r[a, b], v[a, b]) / norm2(r[a, b]);
    return Pi_ab;
  }

}; // class MolteniColagrossiArtificialViscosity

/******************************************************************************\
 ** δ-SPH artificial viscosity (Marrone, 2011).
 ** Weakly-compressible SPH formulation is assumed.
\******************************************************************************/
class DeltaSphArtificialViscosity : public NoArtificialViscosity {
private:

  real_t cs_0_, rho_0_;
  real_t alpha_, delta_;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, grad_rho, h, r, L, v};

  /** Construct artificial viscosity scheme.
   ** @param cs_0 Reference sound speed, as defined for equation of state.
   ** @param rho_0 Reference density, as defined for equation of state.
   ** @param alpha Velocity viscosity coefficient. Typically 0.01~0.05.
   ** @param delta Density diffusion coefficient. Typically 0.1. */
  constexpr DeltaSphArtificialViscosity( //
      real_t cs_0, real_t rho_0,         //
      real_t alpha = 0.02, real_t delta = 0.1) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, alpha_{alpha}, delta_{delta} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(alpha_ > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(delta_ > 0.0, "Density coefficient must be positive.");
  }

  /** Compute continuity equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto const h_ab = h.avg(a, b);
    // Here we assume that density gradients are renormalized because
    // kernel gradient renormalization filter (`L`) was requested.
    auto const D_ab = 2 * rho[a, b] - dot(grad_rho[a] + grad_rho[b], r[a, b]);
    auto const Psi_ab = delta_ * h_ab * cs_0_ * D_ab * r[a, b] / norm2(r[a, b]);
    return Psi_ab;
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto const h_ab = h.avg(a, b);
    auto const Pi_ab = alpha_ * h_ab * cs_0_ * rho_0_ / (rho[a] * rho[b]) *
                       dot(r[a, b], v[a, b]) / norm2(r[a, b]);
    return Pi_ab;
  }

}; // class DeltaSphArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
