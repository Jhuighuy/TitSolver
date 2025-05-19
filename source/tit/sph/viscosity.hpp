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

/// No viscosity, for inviscid flows.
class NoViscosity final {
public:

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{};
  }

  /// Compute viscosity term.
  template<particle_view PV>
  constexpr auto operator()(PV a, PV b) const noexcept -> particle_num_t<PV> {
    TIT_ASSERT(a != b, "Particles must be different!");
    return {};
  }

}; // class NoViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Laplacian viscosity term.
class LaplacianViscosity final {
public:

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{mu};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{rho, r, v};
  }

  /// Compute viscosity term.
  template<particle_view PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto d = r[a].dim();
    const auto mu_ab = mu.havg(a, b);
    return 2 * (d + 2) * mu_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

}; // class ConstantViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity type.
template<class V>
concept viscosity = std::same_as<V, NoViscosity> || //
                    std::same_as<V, LaplacianViscosity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
