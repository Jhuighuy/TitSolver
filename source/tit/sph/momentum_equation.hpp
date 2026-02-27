/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Gravity source.
template<class Num>
class GravitySource final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields = TypeSet{};

  /// Construct the gravity source.
  ///
  /// @param g Gravitational acceleration absolute value.
  constexpr explicit GravitySource(Num g_0) noexcept : g_0_{g_0} {}

  /// Source term value.
  template<particle_view_n<Num, fields> PV>
  constexpr auto operator()(PV /*a*/) const noexcept {
    return unit<1>(particle_vec_t<PV>{}, -g_0_);
  }

private:

  Num g_0_;

}; // class GravitySource

/// Momentum source type.
template<class MS, class Num>
concept momentum_source = std::same_as<MS, GravitySource<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation.
template<class Num,
         viscosity<Num> Viscosity,
         momentum_source<Num>... MomentumSources>
class MomentumEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields =
      Viscosity::fields | (MomentumSources::fields | ... | TypeSet{v, dv_dt});

  /// Construct the momentum equation.
  constexpr explicit MomentumEquation(
      Viscosity viscosity,
      MomentumSources... momentum_sources) noexcept
      : viscosity_{std::move(viscosity)},
        momentum_sources_{std::move(momentum_sources)...} {}

  /// Viscosity term.
  constexpr auto viscosity() const noexcept -> const auto& {
    return viscosity_;
  }

  /// Mass source terms.
  constexpr auto momentum_sources() const noexcept -> const auto& {
    return momentum_sources_;
  }

private:

  [[no_unique_address]] Viscosity viscosity_;
  [[no_unique_address]] std::tuple<MomentumSources...> momentum_sources_;

}; // class MomentumEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation type.
template<class ME, class Num>
concept momentum_equation = specialization_of<ME, MomentumEquation, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
