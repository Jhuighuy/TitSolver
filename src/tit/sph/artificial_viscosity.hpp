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

#include "TitParticle.hpp"
#include "tit/utils/math.hpp"
#include "tit/utils/meta.hpp"
#include "tit/utils/vec.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Zero artificial viscosity scheme.
\******************************************************************************/
class ZeroArtificialViscosity final {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = 0 /*meta::Set{}*/;

  /** Compute artificial kinematic viscosity. */
  template<class PV>
    requires (has<PV>(required_fields))
  static consteval auto kinematic([[maybe_unused]] PV a,
                                  [[maybe_unused]] PV b) noexcept {
    const auto nu_ab = 0.0;
    return nu_ab;
  }

}; // class ZeroArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Alpha-Beta artificial viscosity scheme.
\******************************************************************************/
class AlphaBetaArtificialViscosity final {
private:

  real_t _alpha, _beta, _eps;

public:

  /** Initialize artificial viscosity scheme.
   ** @param alpha First scheme constant.
   ** @param beta Second scheme constant, typically 2x first constant.
   ** @param eps A small number to prevent division by zero. */
  constexpr AlphaBetaArtificialViscosity( //
      real_t alpha = 1.0, real_t beta = 2.0, real_t eps = 0.01) noexcept
      : _alpha{alpha}, _beta{beta}, _eps{eps} {}

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = [] {
    using namespace particle_fields;
    return meta::Set{rho, h, r, v, p, cs};
  }();

  /** Compute artificial kinematic viscosity. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto kinematic(PV a, PV b) const {
    using namespace particle_fields;
    if (dot(r[a, b], v[a, b]) >= 0.0) return 0.0;
    const auto h_ab = avg(h[a], h[b]);
    const auto rho_ab = avg(rho[a], rho[b]);
    const auto cs_ab = avg(cs[a], cs[b]);
    // clang-format off
    const auto mu_ab = h_ab * dot(r[a, b], v[a, b]) /
                              (norm2(r[a, b]) + _eps * pow2(h_ab));
    // clang-format on
    const auto nu_ab = (-_alpha * cs_ab * mu_ab + _beta * pow2(mu_ab)) / rho_ab;
    return nu_ab;
  }

}; // class AlphaBetaArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Artificial viscosity scheme with Balsara switch.
\******************************************************************************/
template<class ArtificialViscosity = AlphaBetaArtificialViscosity>
  requires std::is_object_v<ArtificialViscosity>
class BalsaraArtificialViscosity final {
private:

  ArtificialViscosity _base_viscosity;

public:

  /** Initialize artificial viscosity scheme.
   ** @param base_viscosity Base artificial viscosity scheme. */
  constexpr BalsaraArtificialViscosity(
      ArtificialViscosity base_viscosity = {}) noexcept
      : _base_viscosity{std::move(base_viscosity)} {}

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = [] {
    using namespace particle_fields;
    return meta::Set{h, cs, div_v, curl_v} |
           ArtificialViscosity::required_fields;
  }();

  /** Compute artificial kinematic viscosity. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto kinematic(PV a, PV b) const {
    using namespace particle_fields;
    auto nu_ab = _base_viscosity.kinematic(a, b);
    if (is_zero(nu_ab)) return nu_ab;
    const auto f = [](PV c) {
      return abs(div_v[c]) /
             (abs(div_v[c]) + norm(curl_v[c]) + 0.0001 * cs[c] / h[c]);
    };
    const auto f_ab = avg(f(a), f(b));
    nu_ab *= f_ab;
    return nu_ab;
  }

}; // class BalsaraArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Artificial viscosity scheme with Morris-Monaghan switch.
\******************************************************************************/
template<class ArtificialViscosity = AlphaBetaArtificialViscosity>
  requires std::is_object_v<ArtificialViscosity>
class MorrisMonaghanArtificialViscosity final {
private:

  real_t _alpha_min, _sigma;
  ArtificialViscosity _base_viscosity;

public:

  /** Initialize artificial viscosity scheme.
   ** @param base_viscosity Base artificial viscosity scheme.
   ** @param alpha_min Minimal value of the artificial viscosity switch.
   ** @param sigma Decay time inverse scale factor. */
  constexpr MorrisMonaghanArtificialViscosity(
      ArtificialViscosity base_viscosity = {}, //
      real_t alpha_min = 0.1, real_t sigma = 0.2) noexcept
      : _alpha_min{alpha_min}, _sigma{sigma},
        _base_viscosity{std::move(base_viscosity)} {
    TIT_ASSERT(0.0 <= _alpha_min && _alpha_min <= 1.0,
               "Switch minimal value must be in range [0,1].");
    TIT_ASSERT(_sigma >= 0,
               "Decay time inverse scale factor must be positive.");
  }

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = [] {
    using namespace particle_fields;
    return meta::Set{h, cs, div_v, alpha, dalpha_dt} |
           ArtificialViscosity::required_fields;
  }();

  /** Compute artificial kinematic viscosity. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto kinematic(PV a, PV b) const {
    using namespace particle_fields;
    auto nu_ab = _base_viscosity.kinematic(a, b);
    if (is_zero(nu_ab)) return nu_ab;
    const auto alpha_ab = avg(alpha[a], alpha[b]);
    nu_ab *= alpha_ab;
    return nu_ab;
  }

  /** Compute Morris-Monaghan switch forces. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_switch_deriv(PV a) const {
    using namespace particle_fields;
    const auto S_a = positive(-div_v[a]);
    const auto tau_a = h[a] / (_sigma * cs[a]);
    dalpha_dt[a] = S_a - (alpha[a] - _alpha_min) / tau_a;
  }

}; // class MorrisMonaghanArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
