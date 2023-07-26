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
#include "tit/sph/field.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Ideal gas equation of state.
\******************************************************************************/
class IdealGasEquationOfState {
private:

  real_t _gamma;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, p, cs, eps, deps_dt};

  /** Construct an equation of state.
   ** @param gamma Adiabatic index. */
  constexpr IdealGasEquationOfState(real_t gamma = 1.4) noexcept
      : _gamma{gamma} {
    TIT_ASSERT(_gamma > 1.0, "Adiabatic index must be greater than 1.");
  }

  /** Compute particle pressure (and sound speed). */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = (_gamma - 1.0) * rho[a] * eps[a];
    // The same as sqrt(gamma * p / rho).
    cs[a] = sqrt(_gamma * (_gamma - 1.0) * eps[a]);
  }

}; // class IdealGasEquationOfState

/******************************************************************************\
 ** Adiabatic ideal gas equation of state.
\******************************************************************************/
class AdiabaticIdealGasEquationOfState {
private:

  real_t _kappa, _gamma;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, p, cs};

  /** Construct an equation of state.
   ** @param kappa Thermal conductivity coefficient. (???)
   ** @param gamma Adiabatic index. */
  constexpr AdiabaticIdealGasEquationOfState( //
      real_t kappa = 1.0, real_t gamma = 1.4) noexcept
      : _kappa{kappa}, _gamma{gamma} {
    TIT_ASSERT(_kappa > 0.0,
               "Thermal conductivity coefficient must be positive.");
    TIT_ASSERT(_gamma > 1.0, "Adiabatic index must be greater than 1.");
  }

  /** Compute particle pressure (and sound speed). */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = _kappa * pow(rho[a], _gamma);
    cs[a] = sqrt(_gamma * p[a] / rho[a]);
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

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, p};

  /** Construct an equation of state.
   ** @param cs Reference sound speed, typically 10x of the expected velocity.
   ** @param rho_0 Reference density.
   ** @param p_0 Background pressure.
   ** @param gamma Adiabatic index. */
  constexpr WeaklyCompressibleFluidEquationOfState( //
      real_t cs, real_t rho_0, real_t p_0 = 0.0, real_t gamma = 7.0) noexcept
      : _cs{cs}, _rho_0{rho_0}, _p_0{p_0}, _gamma{gamma} {
    TIT_ASSERT(_cs > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(_rho_0 > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(_gamma > 1.0, "Adiabatic index must be greater than 1.");
  }

  /** Compute particle pressure (and sound speed). */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    const auto p_1 = _rho_0 * pow2(_cs) / _gamma;
    p[a] = _p_0 + p_1 * (pow(rho[a] / _rho_0, _gamma) - 1.0);
    if constexpr (has<PV>(cs)) {
      cs[a] = sqrt(_gamma * (p[a] - _p_0 + p_1) / rho[a]);
    }
  }

}; // class WeaklyCompressibleFluidEquationOfState

/******************************************************************************\
 ** Weakly-compressible fluid equation of state (linear Cole equation).
\******************************************************************************/
class LinearWeaklyCompressibleFluidEquationOfState {
private:

  real_t _cs, _rho_0, _p_0;

public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{rho, p};

  /** Construct an equation of state.
   ** @param cs Reference sound speed, typically 10x of the expected velocity.
   ** @param rho_0 Reference density.
   ** @param p_0 Background pressure. */
  constexpr LinearWeaklyCompressibleFluidEquationOfState( //
      real_t cs, real_t rho_0, real_t p_0 = 0.0) noexcept
      : _cs{cs}, _rho_0{rho_0}, _p_0{p_0} {
    TIT_ASSERT(_cs > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(_rho_0 > 0.0, "Reference density speed must be positive.");
  }

  /** Compute particle pressure (and sound speed). */
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = _p_0 + pow2(_cs) * (rho[a] - _rho_0);
    if constexpr (has<PV>(cs)) {
      // The same as sqrt(gamma * (p - p_0) / rho), where gamma = 1.
      cs[a] = _cs;
    }
  }

}; // class LinearWeaklyCompressibleFluidEquationOfState

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
