/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <nlohmann/json_fwd.hpp>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation.
template<variant SourceTerm>
class ContinuityEquation final {
public:

  /// Construct the continuity equation.
  constexpr explicit ContinuityEquation(SourceTerm source_term) noexcept
      : source_term_{std::move(source_term)} {}

  /// Deserialize from JSON.
  constexpr explicit ContinuityEquation(const nlohmann::json& params)
      : ContinuityEquation{SourceTerm{params.at("source_term")}} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return source_term_.required_uniforms();
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return source_term_.required_varyings() | TypeSet{rho, drho_dt};
  }

  /// Mass source term.
  constexpr auto source_term() const noexcept -> const variant auto& {
    return source_term_;
  }

private:

  [[no_unique_address]] SourceTerm source_term_;

}; // class ContinuityEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation (with runtime dispatch).
template<class Num>
using DContinuityEquation = ContinuityEquation<OptionalVariant<>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation type.
template<class CE>
concept continuity_equation = specialization_of<CE, ContinuityEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
