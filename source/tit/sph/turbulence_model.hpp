/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Turbulence model that contributes no eddy viscosity.
template<class Num>
class NoTurbulenceModel final {
public:

  /// Eddy dynamic viscosity.
  constexpr auto eddy_viscosity(Num /*rho*/, Num /*shear_rate*/) const noexcept
      -> Num {
    return {};
  }

}; // class NoTurbulenceModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Smagorinsky-Lilly large-eddy simulation turbulence model.
template<class Num>
class SmagorinskyLillyTurbulenceModel final {
public:

  /// Construct a Smagorinsky-Lilly turbulence model.
  constexpr explicit SmagorinskyLillyTurbulenceModel(Num C_smag,
                                                     Num length) noexcept
      : C_smag_{C_smag}, length_{length} {
    TIT_ASSERT(C_smag_ >= Num{0}, "Smagorinsky constant must be non-negative!");
    TIT_ASSERT(length_ > Num{0}, "Characteristic length must be positive!");
  }

  /// Eddy dynamic viscosity.
  constexpr auto eddy_viscosity(Num rho, Num shear_rate) const noexcept -> Num {
    return rho * pow2(C_smag_ * length_) * shear_rate;
  }

private:

  Num C_smag_;
  Num length_;

}; // class SmagorinskyLillyTurbulenceModel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Turbulence model type.
template<class TM, class Num>
concept turbulence_model =
    std::same_as<TM, NoTurbulenceModel<Num>> ||
    std::same_as<TM, SmagorinskyLillyTurbulenceModel<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
