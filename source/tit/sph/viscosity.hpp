/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <limits>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Newtonian viscosity with a constant dynamic viscosity coefficient.
template<class Num>
class ConstantViscosity final {
public:

  /// Construct a constant-viscosity model.
  constexpr explicit ConstantViscosity(Num mu) noexcept : mu_{mu} {
    TIT_ASSERT(mu_ >= Num{0}, "Dynamic viscosity must be non-negative!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num /*shear_rate*/) const noexcept
      -> Num {
    return mu_;
  }

private:

  Num mu_;

}; // class ConstantViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reynolds exponential temperature-viscosity law.
template<class Num>
class ReynoldsTemperatureViscosity final {
public:

  /// Construct a Reynolds temperature-viscosity model.
  constexpr explicit ReynoldsTemperatureViscosity(Num mu_0,
                                                  Num T_ref,
                                                  Num beta) noexcept
      : mu_0_{mu_0}, T_ref_{T_ref}, beta_{beta} {
    TIT_ASSERT(mu_0_ >= Num{0}, "Reference viscosity must be non-negative!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num T, Num /*shear_rate*/) const noexcept
      -> Num {
    return mu_0_ * exp(-beta_ * (T - T_ref_));
  }

private:

  Num mu_0_;
  Num T_ref_;
  Num beta_;

}; // class ReynoldsTemperatureViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Arrhenius temperature-viscosity law.
template<class Num>
class ArrheniusViscosity final {
public:

  /// Construct an Arrhenius viscosity model.
  ///
  /// @param mu_ref  Dynamic viscosity at the reference temperature.
  /// @param T_ref   Reference temperature.
  /// @param E_over_R Activation energy divided by the universal gas constant.
  constexpr explicit ArrheniusViscosity(Num mu_ref,
                                        Num T_ref,
                                        Num E_over_R) noexcept
      : mu_ref_{mu_ref}, T_ref_{T_ref}, E_over_R_{E_over_R} {
    TIT_ASSERT(mu_ref_ >= Num{0}, "Reference viscosity must be non-negative!");
    TIT_ASSERT(T_ref_ > Num{0}, "Reference temperature must be positive!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num T, Num /*shear_rate*/) const noexcept
      -> Num {
    return mu_ref_ * exp(E_over_R_ * (inverse(T) - inverse(T_ref_)));
  }

private:

  Num mu_ref_;
  Num T_ref_;
  Num E_over_R_;

}; // class ArrheniusViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Sutherland temperature-viscosity law for gases.
template<class Num>
class SutherlandViscosity final {
public:

  /// Construct a Sutherland viscosity model.
  constexpr explicit SutherlandViscosity(Num mu_ref, Num T_ref, Num S) noexcept
      : mu_ref_{mu_ref}, T_ref_{T_ref}, S_{S} {
    TIT_ASSERT(mu_ref_ >= Num{0}, "Reference viscosity must be non-negative!");
    TIT_ASSERT(T_ref_ > Num{0}, "Reference temperature must be positive!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num T, Num /*shear_rate*/) const noexcept
      -> Num {
    return mu_ref_ * pow(T / T_ref_, Num{1.5}) * (T_ref_ + S_) / (T + S_);
  }

private:

  Num mu_ref_;
  Num T_ref_;
  Num S_;

}; // class SutherlandViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ostwald-de Waele power-law viscosity.
template<class Num>
class PowerLawViscosity final {
public:

  /// Construct a power-law viscosity model.
  constexpr explicit PowerLawViscosity(
      Num consistency,
      Num index,
      Num shear_rate_min = tiny_v<Num>,
      Num mu_min = Num{0},
      Num mu_max = std::numeric_limits<Num>::max()) noexcept
      : consistency_{consistency}, index_{index},
        shear_rate_min_{shear_rate_min}, mu_min_{mu_min}, mu_max_{mu_max} {
    TIT_ASSERT(consistency_ >= Num{0}, "Consistency must be non-negative!");
    TIT_ASSERT(index_ > Num{0}, "Power-law index must be positive!");
    TIT_ASSERT(shear_rate_min_ > Num{0},
               "Minimum shear rate must be positive!");
    TIT_ASSERT(mu_min_ <= mu_max_, "Invalid viscosity bounds!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num shear_rate) const noexcept
      -> Num {
    const auto rate = std::max(shear_rate, shear_rate_min_);
    return std::clamp(consistency_ * pow(rate, index_ - 1), mu_min_, mu_max_);
  }

private:

  Num consistency_;
  Num index_;
  Num shear_rate_min_;
  Num mu_min_;
  Num mu_max_;

}; // class PowerLawViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Carreau-Yasuda shear-thinning viscosity model.
template<class Num>
class CarreauYasudaViscosity final {
public:

  /// Construct a Carreau-Yasuda viscosity model.
  constexpr explicit CarreauYasudaViscosity(Num mu_0,
                                            Num mu_inf,
                                            Num lambda,
                                            Num index,
                                            Num yasuda) noexcept
      : mu_0_{mu_0}, mu_inf_{mu_inf}, lambda_{lambda}, index_{index},
        yasuda_{yasuda} {
    TIT_ASSERT(mu_0_ >= Num{0}, "Zero-shear viscosity must be non-negative!");
    TIT_ASSERT(mu_inf_ >= Num{0},
               "Infinite-shear viscosity must be non-negative!");
    TIT_ASSERT(lambda_ >= Num{0}, "Relaxation time must be non-negative!");
    TIT_ASSERT(yasuda_ > Num{0}, "Yasuda exponent must be positive!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num shear_rate) const noexcept
      -> Num {
    const auto rate_factor = pow(lambda_ * shear_rate, yasuda_);
    return mu_inf_ + (mu_0_ - mu_inf_) *
                         pow(Num{1} + rate_factor, (index_ - 1) / yasuda_);
  }

private:

  Num mu_0_;
  Num mu_inf_;
  Num lambda_;
  Num index_;
  Num yasuda_;

}; // class CarreauYasudaViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cross shear-thinning viscosity model.
template<class Num>
class CrossViscosity final {
public:

  /// Construct a Cross viscosity model.
  constexpr explicit CrossViscosity(Num mu_0,
                                    Num mu_inf,
                                    Num lambda,
                                    Num index) noexcept
      : mu_0_{mu_0}, mu_inf_{mu_inf}, lambda_{lambda}, index_{index} {
    TIT_ASSERT(mu_0_ >= Num{0}, "Zero-shear viscosity must be non-negative!");
    TIT_ASSERT(mu_inf_ >= Num{0},
               "Infinite-shear viscosity must be non-negative!");
    TIT_ASSERT(lambda_ >= Num{0}, "Time constant must be non-negative!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num shear_rate) const noexcept
      -> Num {
    return mu_inf_ +
           (mu_0_ - mu_inf_) / (Num{1} + pow(lambda_ * shear_rate, index_));
  }

private:

  Num mu_0_;
  Num mu_inf_;
  Num lambda_;
  Num index_;

}; // class CrossViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Regularized Bingham plastic viscosity model.
template<class Num>
class BinghamViscosity final {
public:

  /// Construct a Bingham viscosity model.
  constexpr explicit BinghamViscosity(
      Num plastic_mu,
      Num yield_stress,
      Num shear_rate_min = tiny_v<Num>,
      Num mu_max = std::numeric_limits<Num>::max()) noexcept
      : plastic_mu_{plastic_mu}, yield_stress_{yield_stress},
        shear_rate_min_{shear_rate_min}, mu_max_{mu_max} {
    TIT_ASSERT(plastic_mu_ >= Num{0},
               "Plastic viscosity must be non-negative!");
    TIT_ASSERT(yield_stress_ >= Num{0}, "Yield stress must be non-negative!");
    TIT_ASSERT(shear_rate_min_ > Num{0},
               "Minimum shear rate must be positive!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num shear_rate) const noexcept
      -> Num {
    const auto rate = std::max(shear_rate, shear_rate_min_);
    return std::min(plastic_mu_ + yield_stress_ / rate, mu_max_);
  }

private:

  Num plastic_mu_;
  Num yield_stress_;
  Num shear_rate_min_;
  Num mu_max_;

}; // class BinghamViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Regularized Herschel-Bulkley viscosity model.
template<class Num>
class HerschelBulkleyViscosity final {
public:

  /// Construct a Herschel-Bulkley viscosity model.
  constexpr explicit HerschelBulkleyViscosity(
      Num yield_stress,
      Num consistency,
      Num index,
      Num shear_rate_min = tiny_v<Num>,
      Num mu_max = std::numeric_limits<Num>::max()) noexcept
      : yield_stress_{yield_stress}, consistency_{consistency}, index_{index},
        shear_rate_min_{shear_rate_min}, mu_max_{mu_max} {
    TIT_ASSERT(yield_stress_ >= Num{0}, "Yield stress must be non-negative!");
    TIT_ASSERT(consistency_ >= Num{0}, "Consistency must be non-negative!");
    TIT_ASSERT(index_ > Num{0}, "Power-law index must be positive!");
    TIT_ASSERT(shear_rate_min_ > Num{0},
               "Minimum shear rate must be positive!");
  }

  /// Dynamic viscosity.
  constexpr auto dynamic_viscosity(Num /*T*/, Num shear_rate) const noexcept
      -> Num {
    const auto rate = std::max(shear_rate, shear_rate_min_);
    return std::min(yield_stress_ / rate + consistency_ * pow(rate, index_ - 1),
                    mu_max_);
  }

private:

  Num yield_stress_;
  Num consistency_;
  Num index_;
  Num shear_rate_min_;
  Num mu_max_;

}; // class HerschelBulkleyViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity model type.
template<class VM, class Num>
concept viscosity_model = std::same_as<VM, ConstantViscosity<Num>> ||
                          std::same_as<VM, ReynoldsTemperatureViscosity<Num>> ||
                          std::same_as<VM, ArrheniusViscosity<Num>> ||
                          std::same_as<VM, SutherlandViscosity<Num>> ||
                          std::same_as<VM, PowerLawViscosity<Num>> ||
                          std::same_as<VM, CarreauYasudaViscosity<Num>> ||
                          std::same_as<VM, CrossViscosity<Num>> ||
                          std::same_as<VM, BinghamViscosity<Num>> ||
                          std::same_as<VM, HerschelBulkleyViscosity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
