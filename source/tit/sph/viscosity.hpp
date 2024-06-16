/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/meta.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No viscosity, for inviscid flows.
class NoViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Compute viscosity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different.");
    return 0;
  }

}; // class NoViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity with a constant coefficient.
class ConstantViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr meta::Set required_fields{rho, r, v};

  /// Set of particle fields that are modified.
  static constexpr meta::Set modified_fields{/*empty*/};

  /// Construct viscosity with a constant coefficient.
  ///
  /// @param mu Viscosity coefficient.
  constexpr explicit ConstantViscosity(real_t mu) noexcept : mu_{mu} {
    TIT_ASSERT(mu_ > 0.0, "Viscosity coefficient must be positive!");
  }

  /// Compute viscosity term.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto d = r[a].dim();
    return 2 * (d + 2) * mu_ / (rho[a] * rho[b]) * //
           dot(r[a, b], v[a, b]) / (norm2(r[a, b]));
  }

private:

  real_t mu_;

}; // class ConstantViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity type.
template<class V>
concept viscosity = std::same_as<V, NoViscosity> || //
                    std::same_as<V, ConstantViscosity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
