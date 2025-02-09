/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No heat conductivity.
class NoHeatConductivity {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{r};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Heat conductivity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    return zero(r[a, b]);
  }

}; // class NoHeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity term.
class HeatConductivity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, r, v, u, kappa};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct heat conductivity with a constant coefficient.
  ///
  /// @param c_v Specific heat capacity of the fluid.
  constexpr explicit HeatConductivity(real_t c_v) noexcept : c_v_{c_v} {
    TIT_ASSERT(c_v_ > 0.0, "Specific heat capacity must be positive!");
  }

  /// Heat conductivity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto kappa_ab = kappa.havg(a, b);
    return 2 * kappa_ab * u[b, a] * r[a, b] /
           (c_v_ * rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  real_t c_v_;

}; // class HeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity type.
template<class HC>
concept heat_conductivity = std::same_as<HC, NoHeatConductivity> || //
                            std::same_as<HC, HeatConductivity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
