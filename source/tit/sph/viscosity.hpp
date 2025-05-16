/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <variant>

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
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{/*empty*/};
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
  constexpr auto operator()(PV a, PV b) const noexcept -> particle_num_t<PV> {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto d = r[a].dim();
    const auto mu_ab = mu.havg(a, b);
    return 2 * (d + 2) * mu_ab * dot(r[a, b], v[a, b]) /
           (rho[a] * rho[b] * norm2(r[a, b]));
  }

}; // class LaplacianViscosity

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity term variant.
template<class... Impls>
class ViscosityVariant final {
public:

  /// Construct a viscosity term.
  constexpr explicit ViscosityVariant(auto impl) noexcept
      : impls_{std::move(impl)} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return required_uniforms_from_variant(impls_);
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return required_varyings_from_variant(impls_);
  }

  /// Compute viscosity term.
  template<particle_view PV>
  constexpr auto operator()(PV a, PV b) const noexcept -> particle_num_t<PV> {
    return std::visit([a, b](const auto& impl) { return impl(a, b); }, impls_);
  }

private:

  std::variant<Impls...> impls_;

}; // class ViscosityVariant

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Viscosity type.
template<class V>
concept viscosity = std::same_as<V, NoViscosity> || //
                    std::same_as<V, LaplacianViscosity> ||
                    specialization_of<V, ViscosityVariant>;

/// Dynamically dispatched viscosity.
using DViscosity = ViscosityVariant<NoViscosity, LaplacianViscosity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
