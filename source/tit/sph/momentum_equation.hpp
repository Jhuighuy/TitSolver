/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/type_traits.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Gravity source.
class GravitySource final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields = meta::Set{/*empty*/};

  /// Construct the gravity source.
  ///
  /// @param g Gravitational acceleration absolute value.
  constexpr explicit GravitySource(real_t g_0) noexcept : g_0_{g_0} {}

  /// Source term value.
  template<particle_view<required_fields> PV>
  constexpr auto operator()(PV a) const noexcept {
    decltype(auto(v[a])) g{};
    g[1] = -g_0_;
    return g;
  }

private:

  real_t g_0_;

}; // class GravitySource

/// Momentum source type.
template<class MS>
concept momentum_source = std::same_as<MS, GravitySource>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation.
template<momentum_source... MomentumSources>
class MomentumEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields =
      (MomentumSources::required_fields | ... | meta::Set{/*empty*/});

  /// Set of particle fields that are modified.
  static constexpr auto modified_fields =
      (MomentumSources::modified_fields | ... | meta::Set{/*empty*/});

  /// Construct the momentum equation.
  constexpr explicit MomentumEquation(
      MomentumSources... momentum_sources) noexcept
      : momentum_sources_{std::move(momentum_sources)...} {}

  /// Mass source terms.
  constexpr auto momentum_sources() const noexcept -> const auto& {
    return momentum_sources_;
  }

private:

  [[no_unique_address]] std::tuple<MomentumSources...> momentum_sources_;

}; // class MomentumEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation type.
template<class ME>
concept momentum_equation = specialization_of<ME, MomentumEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
