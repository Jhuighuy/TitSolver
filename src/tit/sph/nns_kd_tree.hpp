/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
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
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

#include "TitParticle.hpp" // IWYU pragma: keep
#include "tit/core/assert.hpp"
#include "tit/core/kd_tree.hpp"
#include "tit/core/types.hpp"
#include "tit/sph/field.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** K-dimensional tree-based particle nearest neighbours search algorithm.
\******************************************************************************/
template<class ParticleArray>
  requires std::is_object_v<ParticleArray>
class KDTreeParticleNNS final : public NonCopyable {
private:

  ParticleArray* _particles = nullptr;

  constexpr auto _positions() const noexcept {
    return _particles->views() |
           std::views::transform([](auto a) { return r[a]; });
  }

  using ParticlePositions =
      decltype(std::declval<KDTreeParticleNNS>()._positions());

  KDTree<ParticlePositions> _kd_tree;

public:

  /** Initialize a particle NNS algorithm. */
  /** @{ */
  constexpr KDTreeParticleNNS() noexcept = default;
  constexpr KDTreeParticleNNS(ParticleArray& particles)
      : _particles{&particles}, _kd_tree(_positions()) {}
  /** @} */

  /** Range of particles, that are in search radius to the current one
   ** (including the particle itself). */
  template<class Real>
  constexpr auto nearby(ParticleView<ParticleArray> a,
                        Real search_radius) const {
    TIT_ASSERT(_particles != nullptr, "Particle array was not associated.");
    TIT_ASSERT(_particles == &a.array(),
               "Particle belongs to a different array.");
    TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");
    // Storage for the found indices. It is thread-local (and static),
    // this way we avoid extra reallocations, and the output range remains
    // valid until the next function call.
    thread_local auto b_indices = std::vector<size_t>{};
    b_indices.clear();
    // nanoflann operates on square norms.
    _kd_tree.search(r[a], search_radius, std::back_inserter(b_indices));
    return std::views::all(b_indices) |
           std::views::transform(
               [&](size_t b_index) { return (*_particles)[b_index]; });
  }

}; // class KDTreeParticleNNS

template<class ParticleArray>
KDTreeParticleNNS(ParticleArray&) -> KDTreeParticleNNS<ParticleArray>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
