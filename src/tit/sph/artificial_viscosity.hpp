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
class ZeroArtificialViscosity {
public:

  /** Set of particle variables that are required. */
  using required_variables = meta::Set<>;

  /** Compute artificial kinematic viscosity. */
  template<class ParticleView>
  static consteval auto kinematic([[maybe_unused]] ParticleView a,
                                  [[maybe_unused]] ParticleView b) noexcept {
    return 0.0;
  }

}; // class ZeroArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Alpha-Beta artificial viscosity scheme.
\******************************************************************************/
class AlphaBetaArtificialViscosity {
private:

  real_t _alpha, _beta, _eps;

public:

  /** Initialize artificial viscosity scheme. */
  constexpr AlphaBetaArtificialViscosity( //
      real_t alpha = 1.0, real_t beta = 2.0, real_t eps = 0.01) noexcept
      : _alpha{alpha}, _beta{beta}, _eps{eps} {}

  /** Set of particle variables that are required. */
  using required_variables = decltype([] {
    using namespace particle_variables;
    return meta::Set{rho, h, r, v, p, cs};
  }());

  /** Compute artificial kinematic viscosity. */
  template<has_variables<required_variables> ParticleView>
  constexpr auto kinematic(ParticleView a, ParticleView b) const {
    using namespace particle_variables;
    if (dot(r[a, b], v[a, b]) >= 0.0) return real_t{0.0};
    const auto h_ab = avg(h[a], h[b]);
    const auto rho_ab = avg(rho[a], rho[b]);
    const auto cs_ab = avg(cs[a], cs[b]);
    // clang-format off
    const auto mu_ab = h_ab * dot(r[a, b], v[a, b]) /
                              (norm2(r[a, b]) + _eps * pow2(h_ab));
    // clang-format on
    const auto Pi_ab = (-_alpha * cs_ab * mu_ab + _beta * pow2(mu_ab)) / rho_ab;
    return Pi_ab;
  }

}; // class AlphaBetaArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Artificial viscosity scheme with Balsara switch.
\******************************************************************************/
template<class ArtificialViscosity = AlphaBetaArtificialViscosity>
class BalsaraArtificialViscosity {
private:

  ArtificialViscosity _base_viscosity;

public:

  /** Initialize artificial viscosity scheme. */
  constexpr BalsaraArtificialViscosity(
      ArtificialViscosity base_viscosity = {}) noexcept
      : _base_viscosity{std::move(base_viscosity)} {}

  /** Set of particle variables that are required. */
  using required_variables = decltype([] {
    using namespace particle_variables;
    return meta::Set{h, cs, div_v, curl_v} |
           required_variables_t<ArtificialViscosity>{};
  }());

  /** Compute artificial kinematic viscosity. */
  template<has_variables<required_variables> ParticleView>
  constexpr auto kinematic(ParticleView a, ParticleView b) const {
    using namespace particle_variables;
    const auto Pi_ab = _base_viscosity.kinematic(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    const auto f = [](ParticleView c) {
      return abs(div_v[c]) /
             (abs(div_v[c]) + norm(curl_v[c]) + 0.0001 * cs[c] / h[c]);
    };
    const auto f_ab = avg(f(a), f(b));
    return f_ab * Pi_ab;
  }

}; // class BalsaraArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Artificial viscosity scheme with Morris-Monaghan switch.
\******************************************************************************/
template<class ArtificialViscosity = AlphaBetaArtificialViscosity>
class MorrisMonaghanArtificialViscosity : public AlphaBetaArtificialViscosity {
private:

  real_t _sigma, _alpha_min;
  ArtificialViscosity _base_viscosity;

public:

  /** Initialize artificial viscosity scheme. */
  constexpr MorrisMonaghanArtificialViscosity( //
      real_t sigma = 0.2, real_t alpha_min = 0.1,
      ArtificialViscosity base_viscosity = {}) noexcept
      : _sigma{sigma}, _alpha_min{alpha_min}, //
        _base_viscosity{std::move(base_viscosity)} {}

  /** Set of particle variables that are required. */
  using required_variables = decltype([] -> decltype(auto) {
    using namespace particle_variables;
    return meta::Set{h, cs, div_v, alpha, dalpha_dt} |
           required_variables_t<ArtificialViscosity>{};
  }());

  /** Compute artificial kinematic viscosity. */
  template<has_variables<required_variables> ParticleView>
  constexpr auto kinematic(ParticleView a, ParticleView b) const {
    using namespace particle_variables;
    const auto Pi_ab = _base_viscosity.kinematic(a, b);
    if (is_zero(Pi_ab)) return Pi_ab;
    const auto alpha_ab = avg(alpha[a], alpha[b]);
    return alpha_ab * Pi_ab;
  }

  /** Estimate Morris-Monaghan switch forces. */
  template<has_variables<required_variables> ParticleView>
  constexpr auto switch_deriv(ParticleView a) const {
    using namespace particle_variables;
    const auto S_a = positive(-div_v[a]);
    const auto tau_a = _sigma * h[a] * cs[a];
    return S_a - (alpha[a] - _alpha_min) / tau_a;
  }

}; // class MorrisMonaghanArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
