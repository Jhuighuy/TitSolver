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
#include "tit/utils/vec.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Abstract artificial viscosity estimator.
\******************************************************************************/
template<class Real, dim_t Dim>
class ArtificialViscosity {
public:

  virtual ~ArtificialViscosity() = default;

  /** Compute artificial kinematic viscosity. */
  constexpr virtual Real kinematic(const Particle<Real, Dim>& a,
                                   const Particle<Real, Dim>& b) const = 0;

}; // class TArtificialViscosityEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Dummy artificial viscosity estimator.
\******************************************************************************/
template<class Real, dim_t Dim>
class ZeroArtificialViscosity : public ArtificialViscosity<Real, Dim> {
public:

  /** Compute artificial kinematic viscosity. */
  constexpr Real kinematic( //
      [[maybe_unused]] const Particle<Real, Dim>& a,
      [[maybe_unused]] const Particle<Real, Dim>& b) const override {
    return Real{0.0};
  }

}; // class ZeroArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The Alpha-Beta artificial viscosity estimator.
\******************************************************************************/
template<class Real, dim_t Dim>
class AlphaBetaArtificialViscosity : public ArtificialViscosity<Real, Dim> {
private:

  Real _alpha, _beta, _eps;

public:

  constexpr explicit AlphaBetaArtificialViscosity(
      Real alpha = Real{1.0}, Real beta = Real{2.0},
      Real eps = Real{0.01}) noexcept
      : _alpha{alpha}, _beta{beta}, _eps{eps} {}

  /** Compute artificial kinematic viscosity. */
  constexpr Real kinematic(const Particle<Real, Dim>& a,
                           const Particle<Real, Dim>& b) const override {
    using namespace particle_accessors;
    if (dot(r[a, b], v[a, b]) >= 0.0) return Real{0.0};
    const auto h_ab = avg(h[a], h[b]);
    const auto rho_ab = avg(rho[a], rho[b]);
    const auto cs_ab = avg(cs[a], cs[b]);
    // clang-format off
    const auto mu_ab = h_ab * dot(r[a, b], v[a, b]) / 
                             (norm2(r[a, b]) + _eps * pow2(h_ab));
    // clang-format on
    return (-_alpha * cs_ab * mu_ab + _beta * pow2(mu_ab)) / rho_ab;
  }

}; // class AlphaBetaArtificialViscosity

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
