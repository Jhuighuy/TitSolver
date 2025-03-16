/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/type_utils.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ideal gas equation of state.
class IdealGasEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, u};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct an equation of state.
  ///
  /// @param gamma Adiabatic index.
  constexpr explicit IdealGasEquationOfState(real_t gamma = 1.4) noexcept
      : gamma_{gamma} {
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view<required_fields> PV>
  constexpr auto pressure(PV a) const noexcept {
    return (gamma_ - 1.0) * rho[a] * u[a];
  }

  /// Sound speed value.
  template<particle_view<required_fields> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    return sqrt(gamma_ * (gamma_ - 1.0) * u[a]); // == sqrt(gamma * p / rho).
  }

private:

  real_t gamma_;

}; // class IdealGasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Adiabatic ideal gas equation of state.
class AdiabaticIdealGasEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct an equation of state.
  ///
  /// @param kappa Thermal conductivity coefficient.
  /// @param gamma Adiabatic index.
  constexpr explicit AdiabaticIdealGasEquationOfState(
      real_t kappa = 1.0,
      real_t gamma = 1.4) noexcept
      : kappa_{kappa}, gamma_{gamma} {
    TIT_ASSERT(kappa_ > 0.0, "Conductivity coefficient must be positive!");
    TIT_ASSERT(gamma_ > 1.0, "Adiabatic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view<required_fields> PV>
  constexpr auto pressure(PV a) const noexcept {
    return kappa_ * pow(rho[a], gamma_);
  }

  /// Sound speed value.
  template<particle_view<required_fields> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    return sqrt(kappa_ * pow(rho[a], gamma_)); // == sqrt(gamma * p / rho).
  }

private:

  real_t kappa_;
  real_t gamma_;

}; // class GasEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No pressure-density correction.
class NoCorrection final {
public:

  /// Corrected density value.
  template<particle_view<rho> PV>
  constexpr auto corrected_density(PV a, real_t /*rho_0*/) const noexcept {
    return rho[a];
  }

}; // class NoCorrection

/// Hughes-Graham pressure-density correction (Hughes, Graham, 2010).
class HughesGrahamCorrection final {
public:

  /// Corrected density value.
  template<particle_view<rho> PV>
  constexpr auto corrected_density(PV a, real_t rho_0) const noexcept {
    const auto rho_a = rho[a];
    return a.has_type(ParticleType::fixed) ? std::max(rho_a, rho_0) : rho_a;
  }

}; // class NoCorrection

/// Pressure correction type.
template<class PC>
concept pressure_correction = std::same_as<PC, NoCorrection> || //
                              std::same_as<PC, HughesGrahamCorrection>;

/// Tait equation equation of state (for weakly-compressible fluids).
template<pressure_correction Correction = HughesGrahamCorrection>
class TaitEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct an equation of state.
  ///
  /// @param cs_0       Reference sound speed, typically 10x of the expected
  ///                   velocity.
  /// @param rho_0      Reference density.
  /// @param p_0        Background pressure.
  /// @param gamma      Polytropic index.
  /// @param correction Pressure correction method.
  constexpr explicit TaitEquationOfState(real_t cs_0,
                                         real_t rho_0,
                                         real_t p_0 = 0.0,
                                         real_t gamma = 7.0,
                                         Correction correction = {}) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0}, gamma_{gamma},
        correction_{std::move(correction)} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
    TIT_ASSERT(gamma_ > 1.0, "Polytropic index must be greater than 1!");
  }

  /// Pressure value.
  template<particle_view<required_fields> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto B = rho_0_ * pow2(cs_0_) / gamma_;
    const auto rho_a = correction_.corrected_density(a, rho_0_);
    return p_0_ + B * (pow(rho_a / rho_0_, gamma_) - 1.0);
  }

  /// Sound speed value.
  template<particle_view<required_fields> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    const auto rho_a = correction_.corrected_density(a, rho_0_);
    return cs_0_ * pow(rho_a / rho_0_, gamma_);
  }

private:

  real_t cs_0_;
  real_t rho_0_;
  real_t p_0_;
  real_t gamma_;
  [[no_unique_address]] Correction correction_;

}; // class TaitEquationOfState

/// Linear Tait equation equation of state (for weakly-compressible fluids).
template<pressure_correction Correction = HughesGrahamCorrection>
class LinearTaitEquationOfState final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct an equation of state.
  ///
  /// @param cs_0       Reference sound speed, typically 10x of the expected
  ///                   velocity.
  /// @param rho_0      Reference density.
  /// @param p_0        Background pressure.
  /// @param correction Pressure correction method.
  constexpr LinearTaitEquationOfState(real_t cs_0,
                                      real_t rho_0,
                                      real_t p_0 = 0.0,
                                      Correction correction = {}) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0},
        correction_{std::move(correction)} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
  }

  /// Pressure value.
  template<particle_view<required_fields> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto rho_a = correction_.corrected_density(a, rho_0_);
    return p_0_ + pow2(cs_0_) * (rho_a - rho_0_);
  }

  /// Sound speed value.
  template<particle_view<required_fields> PV>
  constexpr auto sound_speed(PV /*a*/) const noexcept {
    return cs_0_;
  }

private:

  real_t cs_0_;
  real_t rho_0_;
  real_t p_0_;
  [[no_unique_address]] Correction correction_;

}; // class LinearTaitEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Equation of state type.
template<class EOS>
concept equation_of_state =
    std::same_as<EOS, IdealGasEquationOfState> ||
    std::same_as<EOS, AdiabaticIdealGasEquationOfState> ||
    specialization_of<EOS, TaitEquationOfState> ||
    specialization_of<EOS, LinearTaitEquationOfState>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
