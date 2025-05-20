/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <utility>

#include <nlohmann/json_fwd.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No pressure-density correction.
class NoCorrection final {
public:

  /// Default constructor.
  constexpr explicit NoCorrection() = default;

  /// Deserialize from JSON.
  constexpr explicit NoCorrection(const nlohmann::json& /*params*/) noexcept {}

  /// Corrected density value.
  template<particle_view PV>
  constexpr auto corrected_density(PV a, particle_num_t<PV> /*rho_0*/)
      const noexcept {
    return rho[a];
  }

}; // class NoCorrection

/// Hughes-Graham pressure-density correction (Hughes, Graham, 2010).
class HughesGrahamCorrection final {
public:

  /// Default constructor.
  constexpr explicit HughesGrahamCorrection() = default;

  /// Deserialize from JSON.
  constexpr explicit HughesGrahamCorrection(
      const nlohmann::json& /*params*/) noexcept {}

  /// Corrected density value.
  template<particle_view PV>
  constexpr auto corrected_density(PV a,
                                   particle_num_t<PV> rho_0) const noexcept {
    const auto rho_a = rho[a];
    return a.has_type(ParticleType::fixed) ? std::max(rho_a, rho_0) : rho_a;
  }

}; // class NoCorrection

/// Pressure correction (with runtime dispatch).
using DCorrection = Variant<NoCorrection, HughesGrahamCorrection>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tait equation equation of state (for weakly-compressible fluids).
template<class Num, variant Correction>
class TaitEquationOfState final {
public:

  /// Construct an equation of state.
  ///
  /// @param cs_0       Reference sound speed, typically 10x of the expected
  ///                   velocity.
  /// @param rho_0      Reference density.
  /// @param p_0        Background pressure.
  /// @param gamma      Polytropic index.
  /// @param correction Pressure correction method.
  constexpr explicit TaitEquationOfState(Num cs_0,
                                         Num rho_0,
                                         Num p_0 = 0.0,
                                         Num gamma = 7.0,
                                         Correction correction = {}) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0}, gamma_{gamma},
        correction_{std::move(correction)} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
    TIT_ASSERT(gamma_ > 1.0, "Polytropic index must be greater than 1!");
  }

  /// Deserialize from JSON.
  constexpr explicit TaitEquationOfState(const nlohmann::json& params)
      : TaitEquationOfState{Num{params.at("cs_0")},
                            Num{params.at("rho_0")},
                            Num{params.at("p_0")},
                            Num{params.at("gamma")},
                            Correction{params.at("correction")}} {}

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho};
  }

  /// Pressure value.
  template<particle_view_n<Num> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto B = rho_0_ * pow2(cs_0_) / gamma_;
    const auto rho_a = correction_.visit([&](const auto& correction) {
      return correction.corrected_density(a, rho_0_);
    });
    return p_0_ + B * (pow(rho_a / rho_0_, gamma_) - 1.0);
  }

  /// Sound speed value.
  template<particle_view_n<Num> PV>
  constexpr auto sound_speed(PV a) const noexcept {
    const auto rho_a = correction_.corrected_density(a, rho_0_);
    return cs_0_ * pow(rho_a / rho_0_, gamma_);
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num p_0_;
  Num gamma_;
  [[no_unique_address]] Correction correction_;

}; // class TaitEquationOfState

/// Linear Tait equation equation of state (for weakly-compressible fluids).
template<class Num, variant Correction>
class LinearTaitEquationOfState final {
public:

  /// Construct an equation of state.
  ///
  /// @param cs_0       Reference sound speed, typically 10x of the expected
  ///                   velocity.
  /// @param rho_0      Reference density.
  /// @param p_0        Background pressure.
  /// @param correction Pressure correction method.
  constexpr LinearTaitEquationOfState(Num cs_0,
                                      Num rho_0,
                                      Num p_0 = 0.0,
                                      Correction correction = {}) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, p_0_{p_0},
        correction_{std::move(correction)} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
  }

  /// Deserialize from JSON.
  constexpr explicit LinearTaitEquationOfState(const nlohmann::json& params)
      : LinearTaitEquationOfState{Num{params.at("cs_0")},
                                  Num{params.at("rho_0")},
                                  Num{params.at("p_0")},
                                  Correction{params.at("correction")}} {}

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho};
  }

  /// Pressure value.
  template<particle_view_n<Num> PV>
  constexpr auto pressure(PV a) const noexcept {
    const auto rho_a = correction_.visit([&](const auto& correction) {
      return correction.corrected_density(a, rho_0_);
    });
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
  [[no_unique_address]] Correction correction_;

}; // class LinearTaitEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Equation of state (with runtime dispatch).
template<class Num>
using DEquationOfState = Variant<TaitEquationOfState<Num, DCorrection>,
                                 LinearTaitEquationOfState<Num, DCorrection>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
