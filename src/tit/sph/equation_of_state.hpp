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
#include "tit/utils/meta.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Ideal gas equation of state.
\******************************************************************************/
class IdealGasEquationOfState {
private:

  real_t _gamma;

public:

  /** Initialize the equation of state. */
  constexpr IdealGasEquationOfState(real_t gamma = 1.4) noexcept
      : _gamma{gamma} {}

  /** Set of particle fields that is required. */
  using required_fields = decltype([] {
    using namespace particle_fields;
    return meta::Set{rho, p, eps, deps_dt};
  }());

  /** Compute particle pressure. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto pressure(ParticleView a) const noexcept {
    using namespace particle_fields;
    return (_gamma - 1.0) * rho[a] * eps[a];
  }

  /** Compute particle sound speed. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto sound_speed(ParticleView a) const noexcept {
    using namespace particle_fields;
    return sqrt(_gamma * p[a] / rho[a]);
  }

}; // class IdealGasEquationOfState

/******************************************************************************\
 ** Adiabatic ideal gas equation of state.
\******************************************************************************/
class AdiabaticIdealGasEquationOfState {
private:

  real_t _kappa, _gamma;

public:

  /** Initialize the equation of state. */
  constexpr AdiabaticIdealGasEquationOfState( //
      real_t kappa = 1.0, real_t gamma = 1.4) noexcept
      : _kappa{kappa}, _gamma{gamma} {}

  /** Set of particle fields that is required. */
  using required_fields = decltype([] {
    using namespace particle_fields;
    return meta::Set{rho, p};
  }());

  /** Compute particle pressure. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto pressure(ParticleView a) const noexcept {
    using namespace particle_fields;
    return _kappa * pow(rho[a], _gamma);
  }

  /** Compute particle sound speed. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto sound_speed(ParticleView a) const noexcept {
    using namespace particle_fields;
    return sqrt(_gamma * p[a] / rho[a]);
  }

}; // class GasEquationOfState

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Weakly-compressible fluid equation of state (Cole equation).
\******************************************************************************/
class WeaklyCompressibleFluidEquationOfState {
private:

  real_t _cs, _rho_0, _p_0;
  real_t _gamma;

public:

  /** Initialize the equation of state. */
  constexpr WeaklyCompressibleFluidEquationOfState( //
      real_t cs, real_t rho_0, real_t p_0 = 0.0, real_t gamma = 7.0) noexcept
      : _cs{cs}, _rho_0{rho_0}, _p_0{p_0}, _gamma{gamma} {}

  /** Set of particle fields that is required. */
  using required_fields = decltype([] {
    using namespace particle_fields;
    return meta::Set{rho};
  }());

  /** Compute particle pressure. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto pressure(ParticleView a) const noexcept {
    using namespace particle_fields;
    return _p_0 +
           _rho_0 * pow2(_cs) / _gamma * (pow(rho[a] / _rho_0, _gamma) - 1.0);
  }

  /** Compute particle sound speed. */
  template<class ParticleView>
  constexpr auto sound_speed([[maybe_unused]] ParticleView a) const noexcept {
    return _cs;
  }

}; // class WeaklyCompressibleFluidEquationOfState

/******************************************************************************\
 ** Weakly-compressible fluid equation of state (linear).
\******************************************************************************/
class LinearWeaklyCompressibleFluidEquationOfState {
private:

  real_t _cs, _rho_0, _p_0;

public:

  /** Initialize the equation of state. */
  constexpr LinearWeaklyCompressibleFluidEquationOfState( //
      real_t cs, real_t rho_0, real_t p_0 = 0.0) noexcept
      : _cs{cs}, _rho_0{rho_0}, _p_0{p_0} {}

  /** Set of particle fields that is required. */
  using required_fields = decltype([] {
    using namespace particle_fields;
    return meta::Set{rho};
  }());

  /** Compute particle pressure. */
  template<class ParticleView>
    requires particle_view<ParticleView, required_fields>
  constexpr auto pressure(ParticleView a) const noexcept {
    using namespace particle_fields;
    return _p_0 + pow2(_cs) * (rho[a] - _rho_0);
  }

  /** Compute particle sound speed. */
  template<class ParticleView>
  constexpr auto sound_speed([[maybe_unused]] ParticleView a) const noexcept {
    return _cs;
  }

}; // class LinearWeaklyCompressibleFluidEquationOfState

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
