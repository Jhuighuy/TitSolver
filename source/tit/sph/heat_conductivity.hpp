/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No heat conductivity.
class NoHeatConductivity {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{r};

  /// Heat conductivity term.
  template<particle_view<fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    return zero(r[a, b]);
  }

}; // class NoHeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity term.
template<class Num>
class HeatConductivity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, r, v, u};

  /// Construct heat conductivity with a constant coefficient.
  ///
  /// @param kappa Thermal conductivity coefficient.
  /// @param c_v   Specific heat capacity of the fluid.
  constexpr explicit HeatConductivity(Num kappa, Num c_v) noexcept
      : kappa_{kappa}, c_v_{c_v} {
    TIT_ASSERT(kappa_ > 0.0, "Conductivity coefficient must be positive!");
    TIT_ASSERT(c_v_ > 0.0, "Specific heat capacity must be positive!");
  }

  /// Heat conductivity term.
  template<particle_view_n<Num, fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    return 2 * kappa_ * u[b, a] * r[a, b] /
           (c_v_ * rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num kappa_;
  Num c_v_;

}; // class HeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity type.
template<class HC, class Num>
concept heat_conductivity = std::same_as<HC, NoHeatConductivity> ||
                            std::same_as<HC, HeatConductivity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
