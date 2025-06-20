/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/type.hpp"

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
      (MassSources::required_fields | ... | TypeSet{rho, drho_dt});

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      (MassSources::modified_fields | ... | TypeSet{});

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
