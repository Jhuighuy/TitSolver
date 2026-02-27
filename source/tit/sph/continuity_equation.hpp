/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Mass source type.
template<class MS, class Num>
concept mass_source = false; // No mass sources at the moment.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation.
template<class Num, mass_source<Num>... MassSources>
class ContinuityEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields =
      (MassSources::fields | ... | TypeSet{rho, drho_dt});

  /// Construct the continuity equation.
  constexpr explicit ContinuityEquation(MassSources... mass_sources) noexcept
      : mass_sources_{std::move(mass_sources)...} {}

  /// Mass source terms.
  constexpr auto mass_sources() const noexcept -> const auto& {
    return mass_sources_;
  }

private:

  [[no_unique_address]] std::tuple<MassSources...> mass_sources_;

}; // class ContinuityEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation type.
template<class CE, class Num>
concept continuity_equation = specialization_of<CE, ContinuityEquation, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
