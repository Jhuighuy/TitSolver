/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/type.hpp"

#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    return TypeSet{};
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Motion equation.
template<variant ParticleShifting>
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
    return particle_shifting_.required_varyings() | TypeSet{r, v};
  }

  /// Particle shifting method.
  constexpr auto particle_shifting() const noexcept -> const variant auto& {
    return particle_shifting_;
  }

private:

  [[no_unique_address]] ParticleShifting particle_shifting_;

}; // class MotionEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Motion equation type.
template<class ME>
concept motion_equation = specialization_of<ME, MotionEquation>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
