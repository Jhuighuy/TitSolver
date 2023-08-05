/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <concepts>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/types.hpp"
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

  real_t _alpha, _beta;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, h, r, v, p, cs};

  /** Construct artificial viscosity scheme.
   ** @param alpha Linear viscosity coefficient.
   ** @param beta Quadratic viscosity coefficient. Typically two times greater
   **             than linear coefficient for compressible flows and
   **             zero for weakly-compressible or incompressble flows. */
  constexpr AlphaBetaArtificialViscosity( //
      real_t alpha = 1.0, real_t beta = 2.0) noexcept
      : _alpha{alpha}, _beta{beta} {
    TIT_ASSERT(_alpha > 0.0, "Linear coefficient must be positive.");
    TIT_ASSERT(_beta >= 0.0, "Quadratic coefficient must be non-negative.");
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    if (dot(v[a, b], r[a, b]) >= 0.0) return 0.0;
    const auto h_ab = avg(h[a], h[b]);
    const auto rho_ab = avg(rho[a], rho[b]);
    const auto cs_ab = avg(cs[a], cs[b]);
    const auto mu_ab = h_ab * dot(r[a, b], v[a, b]) / norm2(r[a, b]);
    const auto Pi_ab = (-_alpha * cs_ab + _beta * mu_ab) * mu_ab / rho_ab;
    return Pi_ab;
  }

}; // class AlphaBetaArtificialViscosity

/******************************************************************************\
 ** Artificial viscosity with Balsara switch (Balsara, 1995).
\******************************************************************************/
template<class BaseArtificialViscosity = AlphaBetaArtificialViscosity>
  requires std::derived_from<BaseArtificialViscosity, NoArtificialViscosity>
class BalsaraArtificialViscosity : public BaseArtificialViscosity {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, curl_v} |
      BaseArtificialViscosity::required_fields;

  /** Construct artificial viscosity scheme.
   ** @param args Arguments for the base viscosity. */
  template<class... Args>
    requires std::constructible_from<BaseArtificialViscosity, Args...>
  constexpr BalsaraArtificialViscosity(Args&&... args) noexcept
      : BaseArtificialViscosity{std::forward<Args>(args)...} {}

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto Pi_ab = BaseArtificialViscosity::velocity_term(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    const auto f = [](PV c) {
      return abs(div_v[c]) /
             (abs(div_v[c]) + norm(curl_v[c]) + 0.0001 * cs[c] / h[c]);
    };
    const auto f_ab = avg(f(a), f(b));
    Pi_ab *= f_ab;
    return Pi_ab;
  }

}; // class BalsaraArtificialViscosity

/******************************************************************************\
 ** Artificial viscosity with Rosswog switch (Rosswog, 2000).
\******************************************************************************/
template<class BaseArtificialViscosity = BalsaraArtificialViscosity<>>
  requires std::derived_from<BaseArtificialViscosity, NoArtificialViscosity>
class RosswogArtificialViscosity : public BaseArtificialViscosity {
private:

  real_t _alpha_min, _alpha_max;
  real_t _sigma;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, alpha, dalpha_dt} |
      BaseArtificialViscosity::required_fields;

  /** Construct artificial viscosity scheme.
   ** @param alpha_min Minimal value of the switch coefficient.
   ** @param alpha_max Maximal value of the switch coefficient.
   ** @param sigma Decay time inverse scale factor.
   ** @param args Arguments for the base viscosity. */
  template<class... Args>
    requires std::constructible_from<BaseArtificialViscosity, Args...>
  constexpr RosswogArtificialViscosity( //
      real_t alpha_min = 0.1, real_t alpha_max = 1.5, real_t sigma = 0.1,
      Args&&... args) noexcept
      : BaseArtificialViscosity{std::forward<Args>(args)...},
        _alpha_min{alpha_min}, _alpha_max{alpha_max}, _sigma{sigma} {
    TIT_ASSERT(_alpha_min > 0.0, "Switch minimal value must be positive.");
    TIT_ASSERT(_alpha_max > alpha_min, "Switch maximal value must be "
                                       "greater than miminal.");
    TIT_ASSERT(_sigma > 0.0, "Switch decay time inverse scale factor "
                             "must be positive.");
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    auto Pi_ab = BaseArtificialViscosity::velocity_term(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    const auto alpha_ab = avg(alpha[a], alpha[b]);
    Pi_ab *= alpha_ab;
    return Pi_ab;
  }

  /** Compute switch temporal derivative. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_switch_deriv(PV a) const {
    const auto S_a = plus(-div_v[a]);
    const auto tau_a = h[a] / (_sigma * cs[a]);
    dalpha_dt[a] = (_alpha_max - alpha[a]) * S_a - //
                   (alpha[a] - _alpha_min) / tau_a;
  }

}; // class RosswogArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** δ-SPH artificial viscosity (Marrone, 2011).
 ** Continuity equation and weakly-compressible equation of state are assumed.
\******************************************************************************/
class DeltaSPHArtificialViscosity : public NoArtificialViscosity {
private:

  real_t _rho_0, _cs_0;
  real_t _alpha, _delta;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{rho, grad_rho, h, r, S, L, v, cs};

  /** Construct artificial viscosity scheme.
   ** @param cs_0 Reference sound speed, as defined for equation of state.
   ** @param rho_0 Reference density, as defined for equation of state.
   ** @param alpha Velocity viscosity coefficient. Typically 0.01~0.005.
   ** @param delta Density diffusion coefficient. Typically 0.1. */
  constexpr DeltaSPHArtificialViscosity( //
      real_t cs_0, real_t rho_0,         //
      real_t alpha = 0.05, real_t delta = 0.1) noexcept
      : _cs_0{cs_0}, _rho_0{rho_0}, _alpha{alpha}, _delta{delta} {
    TIT_ASSERT(_cs_0 > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(_rho_0 > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(_alpha > 0.0, "Velocity coefficient must be positive.");
    TIT_ASSERT(_delta > 0.0, "Density coefficient must be non-negative.");
  }

  /** Compute continuity equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto density_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    const auto h_ab = avg(h[a], h[b]);
    const auto D_ab = 2 * rho[a, b] - //
                      dot(L[a] * grad_rho[a] + L[b] * grad_rho[b], r[a, b]);
    const auto Psi_ab = _delta * h_ab * _cs_0 * D_ab * r[a, b] / norm2(r[a, b]);
    return Psi_ab;
  }

  /** Compute momentum equation diffusive term. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto velocity_term(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    const auto h_ab = avg(h[a], h[b]);
    const auto Pi_ab = -_alpha * h_ab * _cs_0 * _rho_0 / (rho[a] * rho[b]) *
                       dot(r[a, b], v[a, b]) / norm2(r[a, b]);
    return Pi_ab;
  }

}; // class DeltaSPHArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
