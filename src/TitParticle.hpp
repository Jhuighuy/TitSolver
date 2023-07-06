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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Particle properties accesstors. */
namespace particle_fields {

  /** Particle mass. */
  inline constexpr struct m_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->mass;
    }
  } m;

  /** Particle width. */
  inline constexpr struct h_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->width;
    }
  } h;

  /** Particle density. */
  inline constexpr struct rho_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->density;
    }
  } rho;
  /** WHAT AM I??? */
  inline constexpr struct Omega_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->omega;
    }
  } Omega;

  /** Particle pressure. */
  inline constexpr struct p_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->pressure;
    }
  } p;
  /** Particle sound speed. */
  inline constexpr struct cs_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->sound_speed;
    }
  } cs, sqrt_dp_drho;

  /** Particle internal (thermal) energy. */
  inline constexpr struct eps_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->thermal_energy;
    }
  } eps;
  /** Particle internal (thermal) energy time derivative. */
  inline constexpr struct deps_dt_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->thermal_energy_deriv;
    }
  } deps_dt;

  /** Particle position. */
  inline constexpr struct r_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->position;
    }
#ifndef __INTELLISENSE__
    constexpr auto operator[](auto a, auto b) const noexcept {
      return a->position - b->position;
    }
#endif
  } r;
  /** Particle velocity. */
  inline constexpr struct v_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->velocity;
    }
#ifndef __INTELLISENSE__
    constexpr auto operator[](auto a, auto b) const noexcept {
      return a->velocity - b->velocity;
    }
#endif
  } v, dr_dt;
  /** Particle acceleration. */
  inline constexpr struct a_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->acceleration;
    }
  } a, dv_dt, d2r_dt2;
  /** Particle velocity divergence. */
  inline constexpr struct {
    constexpr auto& operator[](auto a) const noexcept {
      return a->velocity_divergence;
    }
  } div_v;
  /** Particle velocity curl. */
  inline constexpr struct {
    constexpr auto& operator[](auto a) const noexcept {
      return a->velocity_curl;
    }
  } curl_v;

  inline constexpr struct alpha_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->alpha;
    }
  } alpha;
  inline constexpr struct dalpha_dt_t {
    constexpr auto& operator[](auto a) const noexcept {
      return a->alpha_deriv;
    }
  } dalpha_dt;

} // namespace particle_fields

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim>
struct Particle {
  bool fixed;
  Real mass;
  Real width;
  Real density;
  Real omega;
  Real pressure;
  Real sound_speed;
  Real thermal_energy;
  Real thermal_energy_deriv;
  Point<Real, Dim> position;
  Vec<Real, Dim> velocity;
  Vec<Real, Dim> acceleration;
  Real velocity_divergence;
  Vec<Real, 3> velocity_curl;
  Real alpha;
  Real alpha_deriv;
  Real concentration;
  Real chemical_potential;
}; // struct Particle

template<class Real, dim_t Dim>
class ParticleArray : public std::vector<Particle<Real, Dim>> {
public:

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class Func>
  void for_each(const Func& func) {
    std::for_each(this->begin(), this->end(), [&](auto& a) { func(&a); });
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  void sort()
    requires (Dim == 1)
  {
    std::sort(this->begin(), this->end(), [](const auto& a, const auto& b) {
      return a.position < b.position;
    });
  }

  template<class Func>
  void nearby(Particle<Real, Dim>* pa, Real search_radius, const Func& func)
    requires (Dim == 1)
  {
    auto& a = *pa;
    const size_t aIndex = &a - this->data();

    // Neighbours to the left.
    for (size_t bIndex = aIndex - 1; bIndex != SIZE_MAX; --bIndex) {
      Particle<Real, Dim>& b = (*this)[bIndex];
      const Vec<Real, Dim> abDeltaPos = a.position - b.position;
      if (norm(abDeltaPos) > search_radius) break;
      func(&b);
    }

    // Particle itself.
    func(&a);

    // Neighbours to the right.
    for (size_t bIndex = aIndex + 1; bIndex < this->size(); ++bIndex) {
      Particle<Real, Dim>& b = (*this)[bIndex];
      const Vec<Real, Dim> abDeltaPos = a.position - b.position;
      if (norm(abDeltaPos) > search_radius) break;
      func(&b);
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  void print(const char* path) {
    std::ofstream output(path);
    for (const auto& particle : *this) {
      output << particle.position << " ";
      output << particle.density << " ";
      output << particle.velocity << " ";
      output << particle.pressure << " ";
      output << particle.thermal_energy << " ";
      output << particle.width << " ";
      output << particle.alpha << " ";
      output << std::endl;
    }
  }

}; // struct ParticleArray

template<class Real, dim_t Dim>
using ParticleView = Particle<Real, Dim>*;
template<class Real, dim_t Dim>
using ConstParticleView = ParticleView<Real, Dim>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
