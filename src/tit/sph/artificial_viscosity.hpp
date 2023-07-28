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

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Zero artificial viscosity scheme.
\******************************************************************************/
class NoArtificialViscosity final {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{};

  /** Compute artificial viscosity stress tensor flux. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto stress_tensor_flux([[maybe_unused]] PV a,
                                    [[maybe_unused]] PV b) const noexcept {
    const auto Pi_ab = 0.0;
    return Pi_ab;
  }

}; // class NoArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Alpha-Beta artificial viscosity scheme.
\******************************************************************************/
class AlphaBetaArtificialViscosity final {
private:

  real_t _alpha, _beta, _eps;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, h, r, v, p, cs};

  /** Construct artificial viscosity scheme.
   ** @param alpha First scheme constant.
   ** @param beta Second scheme constant, typically 2x first constant.
   ** @param eps A small number to prevent division by zero. */
  constexpr AlphaBetaArtificialViscosity( //
      real_t alpha = 1.0, real_t beta = 2.0, real_t eps = 0.01) noexcept
      : _alpha{alpha}, _beta{beta}, _eps{eps} {
    TIT_ASSERT(alpha > 0.0, "First coefficient must be positive.");
    TIT_ASSERT(beta >= 0.0, "Second coefficient must be non-negative.");
    TIT_ASSERT(eps >= 0.0, "Epsilon must be non-negative.");
  }

  /** Compute artificial viscosity stress tensor flux. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto stress_tensor_flux(PV a, PV b) const {
    if (dot(r[a, b], v[a, b]) >= 0.0) return 0.0;
    const auto h_ab = avg(h[a], h[b]);
    const auto rho_ab = avg(rho[a], rho[b]);
    const auto cs_ab = avg(cs[a], cs[b]);
    // clang-format off
    const auto mu_ab = h_ab * dot(r[a, b], v[a, b]) /
                              (norm2(r[a, b]) + _eps * pow2(h_ab));
    // clang-format on
    const auto Pi_ab = ((-_alpha * cs_ab + _beta * mu_ab) * mu_ab) / rho_ab;
    return Pi_ab;
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

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, curl_v} | ArtificialViscosity::required_fields;

  /** Construct artificial viscosity scheme.
   ** @param base_viscosity Base artificial viscosity scheme. */
  constexpr BalsaraArtificialViscosity(
      ArtificialViscosity base_viscosity = {}) noexcept
      : _base_viscosity{std::move(base_viscosity)} {}

  /** Compute artificial viscosity stress tensor flux. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto stress_tensor_flux(PV a, PV b) const {
    auto Pi_ab = _base_viscosity.stress_tensor_flux(a, b);
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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Artificial viscosity scheme with Morris-Monaghan switch.
\******************************************************************************/
class MorrisMonaghanArtificialViscosity final {
private:

  real_t _alpha_min, _sigma;
  AlphaBetaArtificialViscosity _base_viscosity;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{h, cs, div_v, alpha, dalpha_dt} |
      AlphaBetaArtificialViscosity::required_fields;

  /** Construct artificial viscosity scheme.
   ** @param alpha_min Minimal value of the first
   **                  Alpha-Beta scheme coefficient.
   ** @param sigma Decay time inverse scale factor.
   ** @param beta_alpha_ratio Ratio between the second and the first
   **                         Alpha-Beta scheme coefficients.
   ** @param eps A small number to prevent division by zero. */
  constexpr MorrisMonaghanArtificialViscosity(    //
      real_t alpha_min = 0.1, real_t sigma = 0.2, //
      real_t beta_alpha_ratio = 2.0, real_t eps = 0.01) noexcept
      : _alpha_min{alpha_min}, _sigma{sigma},
        _base_viscosity{/*alpha=*/1.0, /*beta=*/beta_alpha_ratio, eps} {
    TIT_ASSERT(_alpha_min > 0.0,
               "First coefficient minimal value must be positive.");
    TIT_ASSERT(_sigma > 0.0,
               "Decay time inverse scale factor must be positive.");
  }

  /** Compute artificial viscosity stress tensor flux. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto stress_tensor_flux(PV a, PV b) const {
    // Base viscosity was calculated with alpha = 1, so now we
    // multiply it by the true alpha value from the Morris-Monaghan switch
    auto Pi_ab = _base_viscosity.stress_tensor_flux(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    const auto alpha_ab = avg(alpha[a], alpha[b]);
    Pi_ab *= alpha_ab;
    return Pi_ab;
  }

  /** Compute Morris-Monaghan switch forces. */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_switch_deriv(PV a) const {
    const auto S_a = plus(-div_v[a]);
    const auto tau_a = h[a] / (_sigma * cs[a]);
    dalpha_dt[a] = S_a - (alpha[a] - _alpha_min) / tau_a;
  }

}; // class MorrisMonaghanArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
