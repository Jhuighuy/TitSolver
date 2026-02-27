/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/heat_conductivity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy source type.
template<class ES, class Num>
concept energy_source = false; // No energy sources at the moment.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation.
template<class Num,
         heat_conductivity<Num> HeatConductivity,
         energy_source<Num>... EnergySources>
class EnergyEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields =
      HeatConductivity::fields |
      (EnergySources::fields | ... | TypeSet{u, du_dt});

  /// Construct the energy equation.
  constexpr explicit EnergyEquation(HeatConductivity heat_conductivity,
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
template<class EE, class Num>
concept energy_equation = specialization_of<EE, EnergyEquation, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
