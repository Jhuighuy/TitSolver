/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/boundary_extrapolation.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Pressure wall models
//

/// Pressure wall extrapolation with Mayrhofer tangential correction.
template<class Num, std::size_t Order = 2>
class PressureWallModel final {
public:

  /// Extrapolate pressure from nearby fluid samples.
  template<class Samples,
           class XFunc,
           class WeightFunc,
           class PressureFunc,
           class DensityFunc,
           class TangentialOffsetFunc,
           class Vec>
  constexpr auto pressure(Samples&& samples,
                          XFunc&& x_func,
                          WeightFunc&& weight_func,
                          PressureFunc&& pressure_func,
                          DensityFunc&& density_func,
                          TangentialOffsetFunc&& tangential_offset_func,
                          const Vec& normal,
                          const Vec& gravity,
                          Num fallback = {}) const -> Num {
    const auto rho_hat = homogeneous_neumann_extrapolate(samples,
                                                         weight_func,
                                                         density_func,
                                                         Num{});
    if (is_tiny(rho_hat)) return fallback;

    const auto pressure_value = [&](const auto& sample) {
      return pressure_func(sample) -
             density_func(sample) *
                 dot(tangential_offset_func(sample), gravity);
    };
    const auto psi = rho_hat * dot(gravity, normal);
    return robin_extrapolate<Order>(samples,
                                    x_func,
                                    weight_func,
                                    pressure_value,
                                    Num{1},
                                    Num{},
                                    psi,
                                    fallback);
  }

}; // class PressureWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pressure wall model type.
template<class PWM, class Num>
concept pressure_wall_model = std::same_as<PWM, PressureWallModel<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Velocity wall models
//

/// Free-slip velocity wall model.
template<class Num>
class FreeSlipVelocityWallModel final {
public:

  /// Extrapolated wall velocity.
  template<class Samples,
           class XFunc,
           class WeightFunc,
           class VelocityFunc,
           class Vec>
  constexpr auto velocity(Samples&& samples,
                          XFunc&& /*x_func*/,
                          WeightFunc&& weight_func,
                          VelocityFunc&& velocity_func,
                          const Vec& normal) const -> Vec {
    const auto value = homogeneous_neumann_extrapolate(samples,
                                                       weight_func,
                                                       velocity_func,
                                                       Vec{});
    return value - dot(value, normal) * normal;
  }

  /// Physical tangential wall shear stress.
  template<class Vec>
  constexpr auto shear_stress(Num /*rho*/,
                              Num /*mu*/,
                              Num /*distance*/,
                              const Vec& /*v_tangent*/) const noexcept -> Vec {
    return {};
  }

}; // class FreeSlipVelocityWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Laminar no-slip velocity wall model with a stationary wall.
template<class Num>
class LaminarNoSlipVelocityWallModel final {
public:

  /// Extrapolated wall velocity.
  template<class Samples,
           class XFunc,
           class WeightFunc,
           class VelocityFunc,
           class Vec>
  constexpr auto velocity(Samples&& /*samples*/,
                          XFunc&& /*x_func*/,
                          WeightFunc&& /*weight_func*/,
                          VelocityFunc&& /*velocity_func*/,
                          const Vec& /*normal*/) const noexcept -> Vec {
    return {};
  }

  /// Physical tangential wall shear stress.
  template<class Vec>
  constexpr auto shear_stress(Num /*rho*/,
                              Num mu,
                              Num distance,
                              const Vec& v_tangent) const noexcept -> Vec {
    return mu / distance * v_tangent;
  }

}; // class LaminarNoSlipVelocityWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Log-law velocity wall model for high-Reynolds-number near-wall flow.
template<class Num>
class LogLawVelocityWallModel final {
public:

  /// Construct a log-law velocity wall model.
  constexpr explicit LogLawVelocityWallModel(Num kappa = Num{0.41},
                                             Num intercept = Num{5.2},
                                             Num y_plus_laminar = Num{11.0},
                                             std::size_t num_iters = 9) noexcept
      : kappa_{kappa}, intercept_{intercept}, y_plus_laminar_{y_plus_laminar},
        num_iters_{num_iters} {
    TIT_ASSERT(kappa_ > Num{0}, "Von Karman constant must be positive!");
    TIT_ASSERT(y_plus_laminar_ > Num{0},
               "Laminar wall-unit threshold must be positive!");
  }

  /// Extrapolated wall velocity.
  template<class Samples,
           class XFunc,
           class WeightFunc,
           class VelocityFunc,
           class Vec>
  constexpr auto velocity(Samples&& /*samples*/,
                          XFunc&& /*x_func*/,
                          WeightFunc&& /*weight_func*/,
                          VelocityFunc&& /*velocity_func*/,
                          const Vec& /*normal*/) const -> Vec {
    // const auto value = homogeneous_neumann_extrapolate(samples,
    //                                                    weight_func,
    //                                                    velocity_func,
    //                                                    Vec{});
    // return value - dot(value, normal) * normal;
    return {};
  }

  /// Physical tangential wall shear stress.
  template<class Vec>
  constexpr auto shear_stress(Num rho,
                              Num mu,
                              Num distance,
                              const Vec& v_tangent) const noexcept -> Vec {
    const auto speed = norm(v_tangent);
    if (is_tiny(speed)) return {};

    const auto nu = mu / rho;
    if (nu <= Num{}) return {};

    const auto tangent = v_tangent / speed;
    const auto u_tau_laminar = sqrt(speed * nu / distance);
    const auto y_plus_laminar = distance * u_tau_laminar / nu;
    if (y_plus_laminar <= y_plus_laminar_) {
      return rho * pow2(u_tau_laminar) * tangent;
    }

    auto u_tau = u_tau_laminar;
    for (std::size_t i = 0; i < num_iters_; ++i) {
      const auto y_plus = std::max(distance * u_tau / nu, Num{1});
      const auto u_plus = log(y_plus) / kappa_ + intercept_;
      if (is_tiny(u_plus)) break;
      u_tau = speed / u_plus;
    }
    return rho * pow2(u_tau) * tangent;
  }

private:

  Num kappa_;
  Num intercept_;
  Num y_plus_laminar_;
  std::size_t num_iters_;

}; // class LogLawVelocityWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Velocity wall model type.
template<class VWM, class Num>
concept velocity_wall_model =
    std::same_as<VWM, FreeSlipVelocityWallModel<Num>> ||
    std::same_as<VWM, LaminarNoSlipVelocityWallModel<Num>> ||
    std::same_as<VWM, LogLawVelocityWallModel<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Thermal wall models
//

/// Adiabatic thermal wall model.
template<class Num>
class AdiabaticThermalWallModel final {
public:

  /// Extrapolated wall temperature.
  template<class Samples, class XFunc, class WeightFunc, class TemperatureFunc>
  constexpr auto temperature(Samples&& samples,
                             XFunc&& x_func,
                             WeightFunc&& weight_func,
                             TemperatureFunc&& temperature_func,
                             Num /*k*/,
                             Num fallback) const -> Num {
    return robin_extrapolate<2>(samples,
                                x_func,
                                weight_func,
                                temperature_func,
                                Num{1},
                                Num{},
                                Num{},
                                fallback);
  }

  /// Physical heat flux `k dT/dn`.
  template<class Vec>
  constexpr auto heat_flux(Num /*k*/,
                           Num /*T_a*/,
                           Num /*T_s*/,
                           const Vec& /*r_as*/,
                           const Vec& /*normal*/) const noexcept -> Num {
    return {};
  }

}; // class AdiabaticThermalWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fixed-temperature thermal wall model.
template<class Num>
class FixedTemperatureWallModel final {
public:

  /// Construct a fixed-temperature wall model.
  constexpr explicit FixedTemperatureWallModel(Num T_wall) noexcept
      : T_wall_{T_wall} {}

  /// Extrapolated wall temperature.
  template<class Samples, class XFunc, class WeightFunc, class TemperatureFunc>
  constexpr auto temperature(Samples&& /*samples*/,
                             XFunc&& /*x_func*/,
                             WeightFunc&& /*weight_func*/,
                             TemperatureFunc&& /*temperature_func*/,
                             Num /*k*/,
                             Num /*fallback*/) const noexcept -> Num {
    return dirichlet_value(T_wall_);
  }

  /// Physical heat flux `k dT/dn`.
  template<class Vec>
  constexpr auto heat_flux(Num k,
                           Num T_a,
                           Num T_s,
                           const Vec& r_as,
                           const Vec& normal) const noexcept -> Num {
    return k * (T_a - T_s) * dot(r_as, normal) /
           std::max(norm2(r_as), tiny_v<Num>);
  }

private:

  Num T_wall_;

}; // class FixedTemperatureWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Prescribed-heat-flux thermal wall model.
template<class Num>
class HeatFluxThermalWallModel final {
public:

  /// Construct a heat-flux wall model.
  constexpr explicit HeatFluxThermalWallModel(Num q_wall) noexcept
      : q_wall_{q_wall} {}

  /// Extrapolated wall temperature.
  template<class Samples, class XFunc, class WeightFunc, class TemperatureFunc>
  constexpr auto temperature(Samples&& samples,
                             XFunc&& x_func,
                             WeightFunc&& weight_func,
                             TemperatureFunc&& temperature_func,
                             Num k,
                             Num fallback) const -> Num {
    return robin_extrapolate<2>(samples,
                                x_func,
                                weight_func,
                                temperature_func,
                                k,
                                Num{},
                                q_wall_,
                                fallback);
  }

  /// Physical heat flux `k dT/dn`.
  template<class Vec>
  constexpr auto heat_flux(Num /*k*/,
                           Num /*T_a*/,
                           Num /*T_s*/,
                           const Vec& /*r_as*/,
                           const Vec& /*normal*/) const noexcept -> Num {
    return q_wall_;
  }

private:

  Num q_wall_;

}; // class HeatFluxThermalWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Robin thermal wall model: `k dT/dn + lambda T = psi`.
template<class Num>
class RobinThermalWallModel final {
public:

  /// Construct a Robin thermal wall model.
  constexpr explicit RobinThermalWallModel(Num lambda, Num psi) noexcept
      : lambda_{lambda}, psi_{psi} {}

  /// Extrapolated wall temperature.
  template<class Samples, class XFunc, class WeightFunc, class TemperatureFunc>
  constexpr auto temperature(Samples&& samples,
                             XFunc&& x_func,
                             WeightFunc&& weight_func,
                             TemperatureFunc&& temperature_func,
                             Num k,
                             Num fallback) const -> Num {
    return robin_extrapolate<2>(samples,
                                x_func,
                                weight_func,
                                temperature_func,
                                k,
                                lambda_,
                                psi_,
                                fallback);
  }

  /// Physical heat flux `k dT/dn`.
  template<class Vec>
  constexpr auto heat_flux(Num /*k*/,
                           Num /*T_a*/,
                           Num T_s,
                           const Vec& /*r_as*/,
                           const Vec& /*normal*/) const noexcept -> Num {
    return psi_ - lambda_ * T_s;
  }

private:

  Num lambda_;
  Num psi_;

}; // class RobinThermalWallModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Thermal wall model type.
template<class TWM, class Num>
concept thermal_wall_model =
    std::same_as<TWM, AdiabaticThermalWallModel<Num>> ||
    std::same_as<TWM, FixedTemperatureWallModel<Num>> ||
    std::same_as<TWM, HeatFluxThermalWallModel<Num>> ||
    std::same_as<TWM, RobinThermalWallModel<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
