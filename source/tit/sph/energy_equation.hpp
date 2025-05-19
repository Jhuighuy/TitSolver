/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/heat_conductivity.hpp" // IWYU pragma: export

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation.
template<variant HeatConductivity, variant SourceTerm>
class EnergyEquation final {
public:

  /// Construct the energy equation.
  constexpr EnergyEquation(HeatConductivity heat_conductivity,
                           SourceTerm source_term) noexcept
      : heat_conductivity_{std::move(heat_conductivity)},
        source_term_{std::move(source_term)} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return heat_conductivity_.required_uniforms() |
           source_term_.required_uniforms();
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return heat_conductivity_.required_varyings() |
           source_term_.required_varyings() | TypeSet{u, du_dt};
  }

  /// Heat conductivity term.
  constexpr auto heat_conductivity() const noexcept -> const auto& {
    return heat_conductivity_;
  }

  /// Energy source term.
  constexpr auto source_term() const noexcept -> const auto& {
    return source_term_;
  }

private:

  [[no_unique_address]] HeatConductivity heat_conductivity_;
  [[no_unique_address]] SourceTerm source_term_;

}; // class EnergyEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
