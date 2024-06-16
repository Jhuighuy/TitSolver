/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/meta.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No heat conductivity.
class NoHeatConductivity {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Heat conductivity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return 0.0;
  }

}; // class NoHeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity with a constant coefficient.
class ConstantHeatConductivity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, r, v};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct heat conductivity with a constant coefficient.
  ///
  /// @param kappa Heat conductivity coefficient.
  /// @param c_v Specific heat capacity of the fluid.
  constexpr explicit ConstantHeatConductivity(real_t kappa, real_t c_v) noexcept
      : kappa_{kappa}, c_v_{c_v} {
    TIT_ASSERT(kappa_ > 0.0, "Heat conductivity coefficient must be positive!");
    TIT_ASSERT(c_v_ > 0.0, "Specific heat capacity must be positive!");
  }

  /// Heat conductivity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return 4 * kappa_ * u[b, a] / (c_v_ * rho[a] * rho[b] * norm(r[a, b]));
  }

private:

  real_t kappa_ = 10.0;
  real_t c_v_ = 1.0;

}; // class ConstantHeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity type.
template<class HC>
concept heat_conductivity = std::same_as<HC, NoHeatConductivity> || //
                            std::same_as<HC, ConstantHeatConductivity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
