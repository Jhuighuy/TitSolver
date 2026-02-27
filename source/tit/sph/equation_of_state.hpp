/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>

#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ideal gas equation of state.
template<class Num>
class IdealGasEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, u};

  /// Construct an equation of state.
  ///
  /// @param gamma Adiabatic index.
  constexpr explicit IdealGasEquationOfState(Num gamma = 1.4) noexcept
      : gamma_{gamma} {
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view_n<Num, rho, u> PV>
  constexpr auto pressure(PV a) const noexcept {
    return (gamma_ - 1.0) * rho[a] * u[a];
  }

  /// Sound speed value.
  template<particle_view_n<Num, u> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    return sqrt(gamma_ * (gamma_ - 1.0) * u[a]); // == sqrt(gamma * p / rho).
  }

private:

  Num gamma_;

}; // class IdealGasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Adiabatic ideal gas equation of state.
template<class Num>
class AdiabaticIdealGasEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho};

  /// Construct an equation of state.
  ///
  /// @param kappa Thermal conductivity coefficient.
  /// @param gamma Adiabatic index.
  constexpr explicit AdiabaticIdealGasEquationOfState(Num kappa = 1.0,
                                                      Num gamma = 1.4) noexcept
      : kappa_{kappa}, gamma_{gamma} {
    TIT_ASSERT(kappa_ > 0.0, "Conductivity coefficient must be positive!");
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view_n<Num, rho> PV>
  constexpr auto pressure(PV a) const noexcept {
    return kappa_ * pow(rho[a], gamma_);
  }

  /// Sound speed value.
  template<particle_view_n<Num, rho> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    return sqrt(kappa_ * pow(rho[a], gamma_)); // == sqrt(gamma * p / rho).
  }

private:

  Num kappa_;
  Num gamma_;

}; // class GasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tait equation equation of state (for weakly-compressible fluids).
template<class Num>
class TaitEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho};

  /// Construct an equation of state.
  ///
  /// @param cs_0  Reference sound speed, typically 10x of the flow velocity.
  /// @param rho_0 Reference density.
  /// @param p_0   Background pressure.
  /// @param gamma Polytropic index.
  constexpr explicit TaitEquationOfState(Num cs_0,
                                         Num rho_0,
                                         Num p_0 = 0.0,
                                         Num gamma = 7.0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0}, gamma_{gamma} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density must be positive!");
    TIT_ASSERT(gamma_ > 1.0, "Polytropic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view_n<Num, rho> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto B = rho_0_ * pow2(cs_0_) / gamma_;
    const auto rho_a =
        a.has_type(ParticleType::fixed) ? std::max(rho[a], rho_0_) : rho[a];
    return p_0_ + B * (pow(rho_a / rho_0_, gamma_) - 1.0);
  }

  /// Sound speed value.
  template<particle_view_n<Num, rho> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    const auto rho_a =
        a.has_type(ParticleType::fixed) ? std::max(rho[a], rho_0_) : rho[a];
    return cs_0_ * pow(rho_a / rho_0_, gamma_);
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num p_0_;
  Num gamma_;

}; // class TaitEquationOfState

/// Linear Tait equation equation of state (for weakly-compressible fluids).
template<class Num>
class LinearTaitEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho};

  /// Construct an equation of state.
  ///
  /// @param cs_0  Reference sound speed, typically 10x of the flow velocity.
  /// @param rho_0 Reference density.
  /// @param p_0   Background pressure.
  constexpr LinearTaitEquationOfState(Num cs_0,
                                      Num rho_0,
                                      Num p_0 = 0.0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density must be positive!");
  }

  /// Pressure value.
  template<particle_view_n<Num, rho> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto rho_a =
        a.has_type(ParticleType::fixed) ? std::max(rho[a], rho_0_) : rho[a];
    return p_0_ + pow2(cs_0_) * (rho_a - rho_0_);
  }

  /// Sound speed value.
  template<particle_view_n<Num> PV>
  constexpr auto sound_speed(PV /*a*/) const noexcept {
    return cs_0_;
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num p_0_;

}; // class LinearTaitEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Equation of state type.
template<class EOS, class Num>
concept equation_of_state =
    std::same_as<EOS, IdealGasEquationOfState<Num>> ||
    std::same_as<EOS, AdiabaticIdealGasEquationOfState<Num>> ||
    std::same_as<EOS, TaitEquationOfState<Num>> ||
    std::same_as<EOS, LinearTaitEquationOfState<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
