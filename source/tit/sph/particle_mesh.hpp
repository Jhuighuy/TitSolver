/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/inertial_bisection.hpp"
#include "tit/geom/search.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// TODO: move it to an appropriate place!
#if COMPRESSIBLE_SOD_PROBLEM
inline constexpr auto Domain = geom::BBox{Vec{0.0}, Vec{2.0}};
#elif HARD_DAM_BREAKING
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{4.0, 3.0}};
#elif EASY_DAM_BREAKING
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{3.2196, 1.5}};
#else
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{0.0, 0.0}};
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle adjacency graph.
template<class EngineFactory = geom::GridFactory>
  requires std::is_object_v<EngineFactory>
class ParticleMesh final {
public:

  /// Construct a particle adjacency graph.
  ///
  /// @param engine_factory Nearest-neighbors search engine factory.
  constexpr explicit ParticleMesh(EngineFactory engine_factory = {}) noexcept
      : engine_factory_{std::move(engine_factory)} {}

  /// Build an adjacency graph.
  ///
  /// @param radius_func Function that returns search radius for the
  ///                    specified particle view.
  template<class ParticleArray, class SearchRadiusFunc>
  constexpr void build(ParticleArray& particles,
                       const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleAdjacency::build()");
    using PV = ParticleView<ParticleArray>;
    // -------------------------------------------------------------------------
    // STEP I: neighbors search.
    const auto positions = particles[r];
    {
      TIT_PROFILE_SECTION("ParticleAdjacency::search()");
      const auto engine = engine_factory_(positions);
      // -----------------------------------------------------------------------
      static std::vector<std::vector<size_t>> adj_vov{};
      adj_vov.resize(particles.size());
      par::for_each(particles.all(), [&radius_func, &engine](PV a) {
        const auto search_point = r[a];
        const auto search_radius = radius_func(a);
        TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");
        auto& search_results = adj_vov[a.index()];
        search_results.clear();
        engine.search(search_point,
                      search_radius,
                      std::back_inserter(search_results));
      });
      adjacency_.clear();
      for (const auto& adj : adj_vov) adjacency_.push_back(adj);
      adjacency_.sort();
      // -----------------------------------------------------------------------
      static std::vector<std::vector<size_t>> interp_adj_vov{};
      interp_adj_vov.resize(particles.fixed().size());
      par::for_each(
          std::views::enumerate(particles.fixed()),
          [&radius_func, &engine, &particles](auto ia) {
            auto [i, a] = ia;
            const auto search_point = r[a];
            const auto search_radius = 3 * radius_func(a);
            const auto clipped_point = Domain.clamp(search_point);
            const auto interp_point = 2 * clipped_point - search_point;
            auto& search_results = interp_adj_vov[i];
            search_results.clear();
            engine.search(interp_point,
                          search_radius,
                          std::back_inserter(search_results));
            std::erase_if(search_results, [&particles](size_t b_index) {
              return particles.has_type(b_index, ParticleType::fixed);
            });
          });
      interp_adjacency_.clear();
      for (const auto& adj : interp_adj_vov) interp_adjacency_.push_back(adj);
      interp_adjacency_.sort();
    }
    // -------------------------------------------------------------------------
    // STEP II: partitioning.
    size_t nparts = par::num_threads();
    const auto parts = particles[parinfo];
    auto partitioner = geom::InertialBisection(positions, parts, nparts);
    // -------------------------------------------------------------------------
    // STEP III: assembly.
    nparts += 1; // since we have the leftover
    block_adjacency_.assemble_wide(nparts, adjacency_.edges(), [&](auto ij) {
      auto [i, j] = ij;
      return parts[i] == parts[j] ? parts[i] : (nparts - 1);
    });
    TIT_STATS("ParticleAdjacency::block_adjacency_", block_adjacency_.sizes());
  }

  /// Adjacent particles.
  template<class PV>
  constexpr auto operator[](PV a) const noexcept {
    return std::views::all(adjacency_[a.index()]) |
           std::views::transform(
               [a](size_t b_index) { return a.array()[b_index]; });
  }

  /// Adjacent fixed particles.
  template<class PV>
  constexpr auto fixed_interp(PV a) const noexcept {
    TIT_ASSERT(a.has_type(ParticleType::fixed),
               "Particle must be of the fixed type!");
    const auto i = a - *a.array().fixed().begin();
    return std::views::all(interp_adjacency_[i]) |
           std::views::transform(
               [a](size_t b_index) { return a.array()[b_index]; });
  }

  /// Unique pairs of the adjacent particles.
  template<class ParticleArray>
  constexpr auto pairs(ParticleArray& particles) const noexcept {
    return std::views::all(adjacency_.edges()) |
           std::views::transform([&particles](auto ab_indices) {
             const auto [a_index, b_index] = ab_indices;
             return std::tuple{particles[a_index], particles[b_index]};
           });
  }

  template<class ParticleArray>
  constexpr auto block_pairs(ParticleArray& particles) const noexcept {
    return std::views::iota(0UZ, block_adjacency_.size()) |
           std::views::transform([&](size_t block_index) {
             return block_adjacency_[block_index] |
                    std::views::transform([&particles](auto ab_indices) {
                      const auto [a_index, b_index] = ab_indices;
                      return std::tuple{particles[a_index], particles[b_index]};
                    });
           });
  }

private:

  EngineFactory engine_factory_;
  Graph adjacency_;
  Graph interp_adjacency_;
  Multivector<std::tuple<size_t, size_t>> block_adjacency_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
