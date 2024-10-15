/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"

#include "tit/graph/graph.hpp"

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
template<geom::search_func SearchFunc = geom::GridSearch,
         geom::partition_func PartitionFunc = geom::RecursiveInertialBisection,
         geom::partition_func SecondaryPartitionFunc = PartitionFunc>
class ParticleMesh final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle adjacency graph.
  ///
  /// @param search_indexing_func Nearest-neighbors search indexing function.
  /// @param parition_func Geometry partitioning function.
  constexpr explicit ParticleMesh(
      SearchFunc search_func = {},
      PartitionFunc parition_func = {},
      SecondaryPartitionFunc secondary_parition_func = {}) noexcept
      : search_func_{std::move(search_func)},
        parition_func_{std::move(parition_func)},
        secondary_parition_func_{std::move(secondary_parition_func)} {}

  /// Adjacent particles.
  template<particle_view PV>
  constexpr auto operator[](PV a) const noexcept {
    auto& particles = a.array();
    return adjacency_[a.index()] |
           std::views::transform(
               [&particles](size_t b) { return particles[b]; });
  }

  /// Particles used for interpolation for the fixed particles.
  template<particle_view PV>
  constexpr auto fixed_interp(PV a) const noexcept {
    TIT_ASSERT(a.has_type(ParticleType::fixed),
               "Particle must be of the fixed type!");
    auto& particles = a.array();
    const size_t i = a - *particles.fixed().begin();
    return interp_adjacency_[i] | //
           std::views::transform(
               [&particles](size_t b) { return particles[b]; });
  }

  /// Unique pairs of the adjacent particles.
  template<particle_array ParticleArray>
  constexpr auto pairs(ParticleArray& particles) const noexcept {
    return adjacency_.edges() | std::views::transform([&particles](auto ab) {
             const auto [a, b] = ab;
             return std::pair{particles[a], particles[b]};
           });
  }

  /// Unique pairs of the adjacent particles partitioned by the block.
  template<particle_array ParticleArray>
  constexpr auto block_pairs(ParticleArray& particles) const noexcept {
    return block_edges_.buckets() |
           std::views::transform([&particles](auto block) {
             return block | std::views::transform([&particles](auto ab) {
                      const auto [a, b] = ab;
                      /// @todo I have zero idea why, but using pair here
                      /// instead of a tuple causes a massive performance hit.
                      return std::tuple{particles[a], particles[b]};
                    });
           });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Update the adjacency graph.
  template<particle_array ParticleArray, class SearchRadiusFunc>
  constexpr void update(ParticleArray& particles,
                        const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleMesh::update()");

    // Update the adjacency graphs.
    search_(particles, radius_func);

    // Partition the adjacency graph by the block.
    partition_(particles);
  }

private:

  template<particle_array ParticleArray, class SearchRadiusFunc>
  constexpr void search_(ParticleArray& particles,
                         const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleMesh::search()");
    using PV = ParticleView<ParticleArray>;

    // Build the search index.
    const auto positions = r[particles];
    const auto search_index = search_func_(positions);

    // Search for the neighbors.
    static std::vector<std::vector<size_t>> adjacency{};
    adjacency.resize(particles.size());
    par::for_each(particles.all(), [&radius_func, &search_index](PV a) {
      const auto& search_point = r[a];
      const auto search_radius = radius_func(a);
      TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");

      // Search for the neighbors for the current particle and store the
      // sorted results.
      auto& search_results = adjacency[a.index()];
      search_results.clear();
      search_index.search(search_point,
                          search_radius,
                          std::back_inserter(search_results));
      std::ranges::sort(search_results);
    });

    // Compress the adjacency graph.
    adjacency_.clear();
    for (const auto& x : adjacency) adjacency_.push_back(x);

    // Search for the interpolation points for the fixed particles.
    static std::vector<std::vector<size_t>> interp_adjacency{};
    interp_adjacency.resize(particles.fixed().size());
    par::for_each( //
        std::views::enumerate(particles.fixed()),
        [&radius_func, &search_index, &particles](auto ia) {
          auto [i, a] = ia;
          /// @todo Once we have a proper geometry library, we should use
          ///       here and clean up the code.
          const auto& search_point = r[a];
          const auto search_radius = 3 * radius_func(a);
          const auto point_on_boundary = Domain.clamp(search_point);
          const auto interp_point = 2 * point_on_boundary - search_point;

          // Search for the neighbors for the interpolation point and
          // store the sorted results.
          auto& search_results = interp_adjacency[i];
          search_results.clear();
          search_index.search(interp_point,
                              search_radius,
                              std::back_inserter(search_results));
          std::erase_if(search_results, [&particles](size_t b) {
            return particles.has_type(b, ParticleType::fixed);
          });
          std::ranges::sort(search_results);
        });

    // Compress the interpolation graph.
    interp_adjacency_.clear();
    for (const auto& x : interp_adjacency) interp_adjacency_.push_back(x);
  }

  template<particle_array ParticleArray>
  constexpr void partition_(ParticleArray& particles) {
    TIT_PROFILE_SECTION("ParticleMesh::partition()");

    // Build the geometric partitioning.
    constexpr size_t num_levels = 2;
    const auto num_threads = par::num_threads();
    const auto num_parts = num_levels * num_threads + 1;
    const auto positions = r[particles];
    // const auto parts = parinfo[particles];
    static std::vector<Vec<uint8_t, 8>> parts;
    parts.assign(particles.size(),
                 Vec<uint8_t, 8>(static_cast<uint8_t>(num_parts - 1)));
    static std::vector<size_t> interface{};
    interface.clear(), interface.reserve(particles.size());
    for (size_t level = 0; level < num_levels; ++level) {
      const auto first_level = level == 0;
      const auto last_level = level == (num_levels - 1);
      if (first_level) {
        // Do the initial partitioning for all particles.
        parition_func_(positions,
                       parts | //
                           std::views::transform([level](auto& part) -> auto& {
                             return part[level];
                           }),
                       num_threads);

        // Collect the interface particles.
        if (last_level) break;
        std::ranges::copy_if( //
            std::views::iota(size_t{0}, particles.size()),
            std::back_inserter(interface),
            [this](size_t a) { //
              for (const auto b : adjacency_[a]) {
                if (parts[a][0] != parts[b][0]) return true;
              }
              return false;
            });
      } else {
        // Do the partitioning for the interface.
        const auto init_part = level * num_threads;
        secondary_parition_func_(
            interface |
                std::views::transform([positions](size_t a) -> const auto& {
                  return positions[a];
                }),
            interface | std::views::transform([level](size_t a) -> auto& {
              return parts[a][level];
            }),
            num_threads,
            init_part);

        // Update the interface particles.
        if (last_level) break;
        const auto non_interface = std::ranges::partition( //
            interface,
            [level, this](size_t a) {
              for (const auto b : adjacency_[a]) {
                if (parts[a][level] != parts[b][level]) return true;
              }
              return false;
            });
        interface.erase(std::begin(non_interface), std::end(non_interface));
      }
    }

    // Assemble the block adjacency graph.
    block_edges_.assemble_wide( //
        num_parts,
        adjacency_.edges(),
        [num_levels, num_threads, num_parts](const auto& ab) {
          const auto& [a, b] = ab;
          const auto& part_a = parts[a];
          const auto& part_b = parts[b];
          return part_a[find_true(part_a == part_b)];
        });

    // Report the block sizes.
    TIT_STATS("ParticleMesh::block_edges_", block_edges_.sizes());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  graph::Graph adjacency_;
  graph::Graph interp_adjacency_;
  Multivector<std::pair<size_t, size_t>> block_edges_;
  [[no_unique_address]] SearchFunc search_func_;
  [[no_unique_address]] PartitionFunc parition_func_;
  [[no_unique_address]] SecondaryPartitionFunc secondary_parition_func_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
