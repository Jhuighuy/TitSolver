/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No heat conductivity.
class NoHeatConductivity {
public:

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{};
  }

  /// Heat conductivity term.
  template<particle_view PV>
  constexpr auto operator()(PV a, PV b) const noexcept -> particle_vec_t<PV> {
    TIT_ASSERT(a != b, "Particles must be different!");
    return {};
  }

}; // class NoHeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity term.
template<class Num>
class HeatConductivity final {
public:

  /// Construct heat conductivity with a constant coefficient.
  ///
  /// @param c_v Specific heat capacity of the fluid.
  constexpr explicit HeatConductivity(Num c_v) noexcept : c_v_{c_v} {
    TIT_ASSERT(c_v_ > 0.0, "Specific heat capacity must be positive!");
  }

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{kappa};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho, r, v, u};
  }

  /// Heat conductivity term.
  template<particle_view_n<Num> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto kappa_ab = kappa.havg(a, b);
    return 2 * kappa_ab * u[b, a] * r[a, b] /
           (c_v_ * rho[a] * rho[b] * norm2(r[a, b]));
  }

private:

  Num c_v_;

}; // class HeatConductivity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Heat conductivity type.
template<class HC>
concept heat_conductivity = std::same_as<HC, NoHeatConductivity> ||
                            specialization_of<HC, HeatConductivity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
