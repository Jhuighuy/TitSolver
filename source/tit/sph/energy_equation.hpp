/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <optional>
#include <tuple>
#include <utility>

#include "tit/core/type.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/heat_conductivity.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Enegry source type.
template<class ES>
concept energy_source = false; // No energy sources at the moment.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No energy equation.
class NoEnergyEquation {
public:

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{/*empty*/};
  }

}; // class NoEnergyEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation.
template<heat_conductivity HeatConductivity, energy_source... EnergySources>
class EnergyEquation final {
public:

  /// Construct the energy equation.
  constexpr EnergyEquation(HeatConductivity heat_conductivity,
                           EnergySources... energy_sources) noexcept
      : heat_conductivity_{std::move(heat_conductivity)},
        energy_sources_{std::move(energy_sources)...} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return heat_conductivity_.required_uniforms() |
           std::apply(
               [](const auto&... es) {
                 return (es.required_uniforms() | ... | TypeSet{});
               },
               energy_sources_);
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return TypeSet{u, du_dt} | heat_conductivity_.required_varyings() |
           std::apply(
               [](const auto&... es) {
                 return (es.required_varyings() | ... | TypeSet{});
               },
               energy_sources_);
  }

  /// Heat conductivity term.
  constexpr auto heat_conductivity() const noexcept -> const auto& {
    return heat_conductivity_;
  }

  /// Energy source terms.
  constexpr auto energy_sources() const noexcept -> const auto& {
    return energy_sources_;
  }

private:

  [[no_unique_address]] HeatConductivity heat_conductivity_;
  [[no_unique_address]] std::tuple<EnergySources...> energy_sources_;

}; // class EnergyEquation

/// Dynamically dispatched energy equation.
/// @todo What about the energy sources?
template<class Num>
using DEnergyEquation = EnergyEquation<DHeatConductivity<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Optional energy equation.
template<class Impl>
class OptionalEnergyEquation final {
public:

  /// Construct an energy equation variant.
  /// @{
  constexpr OptionalEnergyEquation() = default;
  constexpr explicit OptionalEnergyEquation(Impl impl) noexcept
      : impl_{std::move(impl)} {}
  /// @}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return get_required_uniforms(impl_);
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return get_required_varyings(impl_);
  }

  /// Heat conductivity term.
  constexpr auto heat_conductivity() const noexcept -> const auto& {
    if (impl_.has_value()) return impl_->heat_conductivity();
    std::unreachable();
  }

  /// Energy source terms.
  constexpr auto energy_sources() const noexcept -> const auto& {
    if (impl_.has_value()) return impl_->energy_sources();
    std::unreachable();
  }

private:

  std::optional<Impl> impl_;

}; // class OptionalEnergyEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Optional dynamically dispatched energy equation.
/// @todo What about the energy sources?
template<class Num>
using DOptionalEnergyEquation = OptionalEnergyEquation<DEnergyEquation<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Energy equation type.
template<class EE>
concept energy_equation = std::same_as<EE, NoEnergyEquation> ||
                          specialization_of<EE, EnergyEquation> ||
                          specialization_of<EE, OptionalEnergyEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
