/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <optional>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"

#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No particle shifting.
class NoParticleShifting final {
public:

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{/*empty*/};
  }

}; // class NoParticleShifting

/// Particle shifting technique.
template<class Num>
class ParticleShiftingTechnique final {
public:

  /// Construct the particle shifting technique.
  ///
  /// @param R   `R` parameter. Typically 0.8.
  /// @param Ma  Expected Mach number.
  /// @param CFL Expected CFL number.
  constexpr explicit ParticleShiftingTechnique(Num R, Num Ma, Num CFL) noexcept
      : R_{R}, Ma_{Ma}, CFL_{CFL} {}

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{dr, N, FS};
  }

  /// `R` parameter.
  constexpr auto R() const noexcept -> Num {
    return R_;
  }

  /// Mach number parameter.
  constexpr auto Ma() const noexcept -> Num {
    return Ma_;
  }

  /// CFL number parameter.
  constexpr auto CFL() const noexcept -> Num {
    return CFL_;
  }

private:

  Num R_;
  Num Ma_;
  Num CFL_;

}; // class ParticleShiftingTechnique

/// Optional particle shifting technique.
template<class Impl>
class OptionalParticleShiftingTechnique final {
public:

  /// Construct a particle shifting technique.
  /// @{
  constexpr OptionalParticleShiftingTechnique() = default;
  constexpr explicit OptionalParticleShiftingTechnique(Impl impl) noexcept
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

  /// `R` parameter.
  constexpr auto R() const noexcept {
    TIT_ASSERT(impl_.has_value(), "PST is not set!");
    return impl_->R();
  }

  /// Mach number parameter.
  constexpr auto Ma() const noexcept {
    TIT_ASSERT(impl_.has_value(), "PST is not set!");
    return impl_->Ma();
  }

  /// CFL number parameter.
  constexpr auto CFL() const noexcept {
    TIT_ASSERT(impl_.has_value(), "PST is not set!");
    return impl_->CFL();
  }

private:

  std::optional<Impl> impl_;

}; // class OptionalParticleShiftingTechnique

/// Particle shifting type.
template<class PS>
concept particle_shifting =
    std::same_as<PS, NoParticleShifting> ||
    specialization_of<PS, ParticleShiftingTechnique> ||
    specialization_of<PS, OptionalParticleShiftingTechnique>;

/// Dynamically dispatched particle shifting.
template<class Num>
using DParticleShiftingTechnique =
    OptionalParticleShiftingTechnique<ParticleShiftingTechnique<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Motion equation.
template<particle_shifting ParticleShifting>
class MotionEquation final {
public:

  /// Construct the motion equation.
  constexpr explicit MotionEquation(ParticleShifting particle_shifting) noexcept
      : particle_shifting_{std::move(particle_shifting)} {}

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return particle_shifting_.required_uniforms();
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return particle_shifting_.required_varyings();
  }

  /// Particle shifting method.
  constexpr auto particle_shifting() const noexcept -> const auto& {
    return particle_shifting_;
  }

private:

  [[no_unique_address]] ParticleShifting particle_shifting_;

}; // class MotionEquation

/// Motion equation type.
template<class ME>
concept motion_equation = specialization_of<ME, MotionEquation>;

/// Dynamically dispatched motion equation.
template<class Num>
using DMotionEquation = MotionEquation<DParticleShiftingTechnique<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
