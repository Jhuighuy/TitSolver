/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic summation density.
class SummationDensity {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{};

}; // class SummationDensity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Grad-H summation density.
class GradHSummationDensity : public SummationDensity {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      meta::Set{h, m, r, rho, Omega} | SummationDensity::required_fields;

  /// Construct Grad-H summation density.
  ///
  /// @param eta Coupling factor. Typically 1.0~1.2.
  constexpr explicit GradHSummationDensity(real_t eta = 1.0) noexcept
      : eta_{eta} {}

  /// Particle width with respect to density.
  template<particle_view<required_fields> PV>
  constexpr auto width(PV a) const noexcept {
    const auto d = r[a].dim();
    return eta_ * pow(rho[a] / m[a], -1.0 / d);
  }

  /// Particle density (and it's width derivative) with respect to width.
  template<particle_view<required_fields> PV>
  constexpr auto density(PV a) const noexcept {
    const auto d = r[a].dim();
    const auto Rho_a = m[a] * pow(eta_ / h[a], d);
    const auto dRho_dh_a = -d * Rho_a / h[a];
    return std::tuple{Rho_a, dRho_dh_a};
  }

private:

  real_t eta_;

}; // class GradHSummationDensity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Continuity equation as equation for density.
class ContinuityEquation {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{drho_dt};

}; // class ContinuityEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Density equation type.
template<class DensityEquation>
concept density_equation =
    std::movable<DensityEquation> &&
    (std::derived_from<DensityEquation, SummationDensity> ||
     std::same_as<DensityEquation, ContinuityEquation>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
