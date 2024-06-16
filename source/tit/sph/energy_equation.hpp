/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/meta.hpp"
#include "tit/core/type_traits.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/heat_conductivity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Enegry source type.
template<class ES>
concept energy_source = false; // No energy sources at the moment.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No energy equation.
class NoEnergyEquation {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

}; // class NoEnergyEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation.
template<heat_conductivity HeatConductivity, energy_source... EnergySources>
class EnergyEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      HeatConductivity::required_fields |
      (EnergySources::required_fields | ... | meta::Set{u, du_dt});

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      HeatConductivity::modified_fields |
      (EnergySources::modified_fields | ... | meta::Set{/*empty*/});

  /// Construct the energy equation.
  constexpr EnergyEquation(HeatConductivity heat_conductivity,
                           EnergySources... energy_sources) noexcept
      : heat_conductivity_{std::move(heat_conductivity)},
        energy_sources_{std::move(energy_sources)...} {}

  /// Heat conductivity term.
  constexpr auto heat_conductivity() const noexcept -> const auto& {
    return heat_conductivity_;
  }

  /// Energy source terms.
  constexpr auto energy_sources() const noexcept -> const auto& {
    return energy_sources_;
  }

private:

  [[no_unique_address]] HeatConductivity heat_conductivity_;
  [[no_unique_address]] std::tuple<EnergySources...> energy_sources_;

}; // class EnergyEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation type.
template<class EE>
concept energy_equation = std::same_as<EE, NoEnergyEquation> || //
                          specialization_of<EE, EnergyEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
