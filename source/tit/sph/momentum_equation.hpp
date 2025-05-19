/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/type.hpp"

#include "tit/sph/artificial_viscosity.hpp" // IWYU pragma: export
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/viscosity.hpp" // IWYU pragma: export

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
    return TypeSet{};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{};
  }

  /// Source term value.
  template<particle_view_n<Num> PV>
  constexpr auto operator()(PV /*a*/) const noexcept {
    return unit<1>(particle_vec_t<PV>{}, -g_0_);
  }

private:

  Num g_0_;

}; // class GravitySource

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation.
template<variant Viscosity, variant ArtificialViscosity, variant SourceTerm>
class MomentumEquation final {
public:

  /// Construct the momentum equation.
  constexpr explicit MomentumEquation(Viscosity viscosity,
                                      ArtificialViscosity artificial_viscosity,
                                      SourceTerm source_term) noexcept
      : viscosity_{std::move(viscosity)},
        artificial_viscosity_{std::move(artificial_viscosity)},
        source_term_{std::move(source_term)} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return viscosity_.required_uniforms() |
           artificial_viscosity_.required_uniforms() |
           source_term_.required_uniforms();
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return viscosity_.required_varyings() |
           artificial_viscosity_.required_varyings() |
           source_term_.required_varyings() | TypeSet{v, dv_dt};
  }

  /// Viscosity term.
  constexpr auto viscosity() const noexcept -> const variant auto& {
    return viscosity_;
  }

  /// Artificial viscosity term.
  constexpr auto artificial_viscosity() const noexcept -> const variant auto& {
    return artificial_viscosity_;
  }

  /// Mass source term.
  constexpr auto source_term() const noexcept -> const variant auto& {
    return source_term_;
  }

private:

  [[no_unique_address]] Viscosity viscosity_;
  [[no_unique_address]] ArtificialViscosity artificial_viscosity_;
  [[no_unique_address]] SourceTerm source_term_;

}; // class MomentumEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Momentum equation type.
template<class ME>
concept momentum_equation = specialization_of<ME, MomentumEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
