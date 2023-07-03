/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <execution>

#include "tit/utils/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim>
class Particle {
public:

  /** Particle mass. */
  Real mass;

  /** Particle width. */
  Real width;

  /** Particle density. */
  Real density;
  /** Particle density derivative with respect to particle width. */
  Real density_width_deriv;

  /** Particle pressure. */
  Real pressure;

  /** Particle sound speed. */
  Real sound_speed;

  /** Particle internal (thermal) energy. */
  Real thermal_energy;
  /** Particle internal (thermal) energy time derivative. */
  Real thermal_energy_deriv;

  /** Perticle position. */
  Point<Real, Dim> position;
  /** Perticle velocity. */
  Vec<Real, Dim> velocity;
  /** Perticle acceleration. */
  Vec<Real, Dim> acceleration;

}; // struct Particle

namespace particle_accessors {

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.mass;
    }
  } m;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.width;
    }
  } h;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.density;
    }
  } rho;
  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.density_width_deriv;
    }
  } drho_dh;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.pressure;
    }
  } p;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.sound_speed;
    }
  } cs;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.thermal_energy;
    }
  } eps;
  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.thermal_energy_deriv;
    }
  } deps_dt;

  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.position;
    }
    constexpr auto operator[](auto& a, auto& b) const noexcept {
      return (*this)[a] - (*this)[b];
    }
  } r;
  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.velocity;
    }
    constexpr auto operator[](auto& a, auto& b) const noexcept {
      return (*this)[a] - (*this)[b];
    }
  } v, dr_dt;
  inline constexpr struct {
    constexpr auto& operator[](auto& a) const noexcept {
      return a.acceleration;
    }
  } a, dv_dt, d2r_dt2;

} // namespace particle_accessors

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim>
struct ParticleArray {
  Particle<Real, Dim>* Particles;
  size_t NumParticles;

  void SortParticles() {
    std::sort(Particles, Particles + NumParticles,
              [](const Particle<Real, Dim>& a, const Particle<Real, Dim>& b) {
                return a.position < b.position;
              });
  }

  template<class Func>
  void for_each(const Func& func) {
    std::for_each(Particles, Particles + NumParticles, func);
  }

  template<class Func>
  void nearby(Particle<Real, Dim>& a, Real search_width, const Func& func) {
    const size_t aIndex = &a - Particles;

    // Neighbours to the left.
    for (size_t bIndex = aIndex - 1; bIndex != SIZE_MAX; --bIndex) {
      Particle<Real, Dim>& b = Particles[bIndex];
      const Vec<Real, Dim> abDeltaPos = a.position - b.position;
      if (norm(abDeltaPos) > search_width) break;
      func(b);
    }

    // Particle itself.
    func(a);

    // Neighbours to the right.
    for (size_t bIndex = aIndex + 1; bIndex < NumParticles; ++bIndex) {
      Particle<Real, Dim>& b = Particles[bIndex];
      const Vec<Real, Dim> abDeltaPos = a.position - b.position;
      if (norm(abDeltaPos) > search_width) break;
      func(b);
    }
  }

}; // struct ParticleArray

template<class func_t, class Real, dim_t Dim>
void ForEach(ParticleArray<Real, Dim>& particles, const func_t& func) {
  particles.for_each(func);
}

template<class func_t, class Real, dim_t Dim>
void ForEachNeighbour(ParticleArray<Real, Dim>& particles,
                      Particle<Real, Dim>& aParticle, Real search_width,
                      const func_t& func) {
  particles.for_each_neighbour(aParticle, search_width, func);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit