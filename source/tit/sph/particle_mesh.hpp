/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle adjacency graph.
template<geom::search_func SearchFunc, geom::face_search_func FaceSearchFunc>
class ParticleMesh final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle adjacency graph.
  ///
  /// @param search_func Nearest-neighbors search indexing function.
  /// @param face_search_func Face search function.
  constexpr explicit ParticleMesh(SearchFunc search_func = {},
                                  FaceSearchFunc face_search_func = {}) noexcept
      : search_func_{std::move(search_func)},
        face_search_func_{std::move(face_search_func)} {}

  /// Adjacent particles.
  template<particle_view PV>
  constexpr auto operator[](PV a) const noexcept {
    auto& particles = a.array();
    TIT_ASSERT(a.index() < adjacency_.size(),
               "Particle is not a local mesh target.");
    return adjacency_[a.index()] |
           std::views::transform(
               [&particles](std::size_t b) { return particles[b]; });
  }

  /// Adjacent faces.
  template<class Domain, particle_view PV>
  constexpr auto operator[](const Domain& domain, PV a) const noexcept {
    auto& particles = a.array();
    TIT_ASSERT(a.index() < face_adjacency_.size(),
               "Particle is not a local mesh target.");
    return face_adjacency_[a.index()] |
           std::views::transform([&domain, &particles](std::size_t face_index) {
             const auto& [... vert_indices] = domain.face_verts(face_index);
             return std::pair{domain.face(face_index),
                              std::tuple{particles.fixed()[vert_indices]...}};
           });
  }

  /// Update target adjacency rows over all process-local particles.
  template<class Domain, particle_array ParticleArray, class SearchRadiusFunc>
  void update(const Domain& domain,
              ParticleArray& particles,
              const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleMesh::update()");

    search_(domain, particles, radius_func);
  }

private:

  template<class Domain, particle_array ParticleArray, class SearchRadiusFunc>
  void search_(const Domain& domain,
               ParticleArray& particles,
               const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleMesh::search()");
    using PV = ParticleView<ParticleArray>;

    // Build the search index.
    const auto positions = r[particles];
    const auto search_index = search_func_(positions);
    const auto face_index = face_search_func_(domain);

    // Search for owned and boundary target neighbors. Ghost particles are
    // indexed and may appear as neighbors, but never receive adjacency rows.
    adjacency_.resize(std::ranges::size(particles.active()));
    par::for_each(
        particles.active(),
        [&particles, &radius_func, &search_index, this](PV a) {
          const auto& search_point = r[a];
          const auto search_radius = radius_func(a);
          TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");

          auto& search_results = adjacency_[a.index()];
          search_results.clear();
          search_index.search(geom::BSphere{search_point, search_radius},
                              std::back_inserter(search_results));
          std::erase(search_results, a.index());
          // Stable IDs make accumulation order independent of the local
          // owned/fixed/ghost storage layout and therefore of the rank count.
          std::ranges::sort(search_results,
                            {},
                            [&particles](std::size_t index) {
                              return particles[index].id();
                            });
        });

    // Search for adjacent boundary faces for the same target set.
    face_adjacency_.resize(std::ranges::size(particles.active()));
    par::for_each(particles.active(), [&radius_func, &face_index, this](PV a) {
      const auto& search_point = r[a];
      const auto search_radius = radius_func(a);
      TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");

      auto& face_results = face_adjacency_[a.index()];
      face_results.clear();
      face_index.search(geom::BSphere{search_point, search_radius},
                        std::back_inserter(face_results));
      std::ranges::sort(face_results);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  std::vector<std::vector<std::size_t>> adjacency_;
  std::vector<std::vector<std::size_t>> face_adjacency_;
  [[no_unique_address]] SearchFunc search_func_;
  [[no_unique_address]] FaceSearchFunc face_search_func_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
