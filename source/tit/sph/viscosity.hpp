/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No viscosity, for inviscid flows.
class NoViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{/*empty*/};

  /// Compute viscosity term.
  template<particle_view<fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    return zero(rho[a, b]);
  }

}; // class NoViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Laplacian viscosity term.
template<class Num>
class LaplacianViscosity final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{rho, r, v};

  /// Construct Laplacian viscosity.
  constexpr explicit LaplacianViscosity(Num mu) noexcept : mu_{mu} {}

  /// Compute viscosity term.
  template<particle_view<fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    const auto d = r[a].dim();
    return 2 * (d + 2) * mu_ * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num mu_;

}; // class LaplacianViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity type.
template<class V, class Num>
concept viscosity = std::same_as<V, NoViscosity> || //
                    std::same_as<V, LaplacianViscosity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
