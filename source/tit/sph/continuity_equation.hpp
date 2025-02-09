/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/meta.hpp"
#include "tit/core/type_utils.hpp"

#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Mass source type.
template<class MS>
concept mass_source = false; // No mass sources at the moment.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation.
template<mass_source... MassSources>
class ContinuityEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      (MassSources::required_fields | ... | meta::Set{rho, drho_dt});

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      (MassSources::modified_fields | ... | meta::Set{/*empty*/});

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
template<class CE>
concept continuity_equation = specialization_of<CE, ContinuityEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
