/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"

#include "tit/sph/field.hpp"

namespace tit::sph {

// A placeholder for now.
template<class EquationOfState>
concept equation_of_state = std::movable<EquationOfState>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ideal gas equation of state.
class IdealGasEquationOfState {
private:

  real_t gamma_;

public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{rho, p, cs, u, du_dt};

  /// Construct an equation of state.
  ///
  /// @param gamma Adiabatic index.
  constexpr explicit IdealGasEquationOfState(real_t gamma = 1.4) noexcept
      : gamma_{gamma} {
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1.");
  }

  /// Compute particle pressure (and sound speed).
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = (gamma_ - 1.0) * rho[a] * u[a];
    // The same as sqrt(gamma * p / rho).
    cs[a] = sqrt(gamma_ * (gamma_ - 1.0) * u[a]);
  }

}; // class IdealGasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Adiabatic ideal gas equation of state.
class AdiabaticIdealGasEquationOfState {
private:

  real_t kappa_, gamma_;

public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{rho, p, cs};

  /// Construct an equation of state.
  ///
  /// @param kappa Thermal conductivity coefficient. (???)
  /// @param gamma Adiabatic index.
  constexpr explicit AdiabaticIdealGasEquationOfState( //
      real_t kappa = 1.0, real_t gamma = 1.4) noexcept
      : kappa_{kappa}, gamma_{gamma} {
    TIT_ASSERT(kappa_ > 0.0,
               "Thermal conductivity coefficient must be positive.");
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1.");
  }

  /// Compute particle pressure (and sound speed).
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = kappa_ * pow(rho[a], gamma_);
    cs[a] = sqrt(gamma_ * p[a] / rho[a]);
  }

}; // class GasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Weakly-compressible fluid equation of state (Cole equation).
class WeaklyCompressibleFluidEquationOfState {
private:

  real_t cs_0_, rho_0_, p_0_;
  real_t gamma_;

public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{rho, p};

  /// Construct an equation of state.
  ///
  /// @param cs_0 Reference sound speed, typically 10x of the expected velocity.
  /// @param rho_0 Reference density.
  /// @param p_0 Background pressure.
  /// @param gamma Adiabatic index.
  constexpr WeaklyCompressibleFluidEquationOfState( //
      real_t cs_0, real_t rho_0, real_t p_0 = 0.0, real_t gamma = 7.0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0}, gamma_{gamma} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1.");
  }

  /// Compute particle pressure (and sound speed).
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    auto const p_1 = rho_0_ * pow2(cs_0_) / gamma_;
    p[a] = p_0_ + p_1 * (pow(rho[a] / rho_0_, gamma_) - 1.0);
    if constexpr (has<PV>(cs)) {
      cs[a] = sqrt(gamma_ * (p[a] - p_0_ + p_1) / rho[a]);
    }
  }

}; // class WeaklyCompressibleFluidEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Weakly-compressible fluid equation of state (linear Cole equation).
class LinearWeaklyCompressibleFluidEquationOfState {
private:

  real_t cs_0_, rho_0_, p_0_;

public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{rho, p};

  /// Construct an equation of state.
  ///
  /// @param cs_0 Reference sound speed, typically 10x of the expected velocity.
  /// @param rho_0 Reference density.
  /// @param p_0 Background pressure.
  constexpr LinearWeaklyCompressibleFluidEquationOfState( //
      real_t cs_0, real_t rho_0, real_t p_0 = 0.0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive.");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive.");
  }

  /// Compute particle pressure (and sound speed).
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr void compute_pressure(PV a) const noexcept {
    p[a] = p_0_ + pow2(cs_0_) * (rho[a] - rho_0_);
    if constexpr (has<PV>(cs)) {
      // The same as sqrt(gamma * (p - p_0) / rho), where gamma = 1.
      cs[a] = cs_0_;
    }
  }

}; // class LinearWeaklyCompressibleFluidEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
