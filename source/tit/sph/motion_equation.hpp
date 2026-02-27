/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No particle shifting.
class NoParticleShifting final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields = TypeSet{};

}; // class NoParticleShifting

/// Particle shifting technique.
template<class Num>
class ParticleShiftingTechnique final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields = TypeSet{dr, N, FS};

  /// Construct the particle shifting technique.
  ///
  /// @param R   `R` parameter. Typically 0.8.
  /// @param Ma  Expected Mach number.
  /// @param CFL Expected CFL number.
  constexpr explicit ParticleShiftingTechnique(Num R, Num Ma, Num CFL) noexcept
      : R_{R}, Ma_{Ma}, CFL_{CFL} {}

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

/// Particle shifting type.
template<class PS, class Num>
concept particle_shifting = std::same_as<PS, NoParticleShifting> ||
                            std::same_as<PS, ParticleShiftingTechnique<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Motion equation.
template<class Num, particle_shifting<Num> ParticleShifting>
class MotionEquation final {
public:

  /// Set of particle fields that are required.
  static constexpr auto fields = ParticleShifting::fields | TypeSet{r, v};

  /// Construct the motion equation.
  constexpr explicit MotionEquation(ParticleShifting particle_shifting) noexcept
      : particle_shifting_{std::move(particle_shifting)} {}

  /// Particle shifting method.
  constexpr auto particle_shifting() const noexcept -> const auto& {
    return particle_shifting_;
  }

private:

  [[no_unique_address]] ParticleShifting particle_shifting_;

}; // class MotionEquation

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Motion equation type.
template<class ME, class Num>
concept motion_equation = specialization_of<ME, MotionEquation, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
