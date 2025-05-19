/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/type.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Gravity source.
template<class Num>
class GravitySource final {
public:

  /// Construct the gravity source.
  ///
  /// @param g Gravitational acceleration absolute value.
  constexpr explicit GravitySource(Num g_0) noexcept : g_0_{g_0} {}

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Source term value.
  template<particle_view_n<Num> PV>
  constexpr auto operator()(PV /*a*/) const noexcept {
    return unit<1>(particle_vec_t<PV>{}, -g_0_);
  }

private:

  Num g_0_;

}; // class GravitySource

/// Momentum source type.
template<class MS>
concept momentum_source = specialization_of<MS, GravitySource>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation.
template<viscosity Viscosity,
         artificial_viscosity ArtificialViscosity,
         momentum_source... MomentumSources>
class MomentumEquation final {
public:

  /// Construct the momentum equation.
  constexpr explicit MomentumEquation(
      Viscosity viscosity,
      ArtificialViscosity artificial_viscosity,
      MomentumSources... momentum_sources) noexcept
      : viscosity_{std::move(viscosity)},
        artificial_viscosity_{std::move(artificial_viscosity)},
        momentum_sources_{std::move(momentum_sources)...} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return viscosity_.required_uniforms() |
           artificial_viscosity_.required_uniforms() |
           std::apply(
               [](const auto&... f) {
                 return (f.required_uniforms() | ... | TypeSet{/*empty*/});
               },
               momentum_sources_);
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return viscosity_.required_varyings() |
           artificial_viscosity_.required_varyings() |
           std::apply(
               [](const auto&... f) {
                 return (f.required_varyings() | ... | TypeSet{/*empty*/});
               },
               momentum_sources_) |
           TypeSet{v, dv_dt};
  }

  /// Viscosity term.
  constexpr auto viscosity() const noexcept -> const auto& {
    return viscosity_;
  }

  /// Artificial viscosity term.
  constexpr auto artificial_viscosity() const noexcept -> const auto& {
    return artificial_viscosity_;
  }

  /// Mass source terms.
  constexpr auto momentum_sources() const noexcept -> const auto& {
    return momentum_sources_;
  }

private:

  [[no_unique_address]] Viscosity viscosity_;
  [[no_unique_address]] ArtificialViscosity artificial_viscosity_;
  [[no_unique_address]] std::tuple<MomentumSources...> momentum_sources_;

}; // class MomentumEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation type.
template<class ME>
concept momentum_equation = specialization_of<ME, MomentumEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
