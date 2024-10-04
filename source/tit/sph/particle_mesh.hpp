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
#include "tit/geom/partitioning.hpp"
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
template<geom::search_factory SearchFactory = geom::GridFactory,
         geom::partitioning_factory PrimaryPartitioningFactory =
             geom::InertialBisectionFactory,
         geom::partitioning_factory SecondaryPartitioningFactory =
             geom::GraphBasedPartitioningFactory>
class ParticleMesh final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle adjacency graph.
  ///
  /// @param search_factory Nearest-neighbors search factory.
  /// @param partitioning_factory Geometry partitioning factory.
  constexpr explicit ParticleMesh(
      SearchFactory search_factory = {},
      PrimaryPartitioningFactory primary_partitioning_factory = {},
      SecondaryPartitioningFactory secondary_partitioning_factory = {})
      : search_factory_{std::move(search_factory)},
        primary_partitioning_factory_{std::move(primary_partitioning_factory)},
        secondary_partitioning_factory_{
            std::move(secondary_partitioning_factory)} {}

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
    const auto search_index = search_factory_(positions);

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
    const auto& adjacency = adjacency_;
    const auto& secondary_partitioning_factory =
        secondary_partitioning_factory_;

    // Partition the particles adjacency graph.
    static std::vector<std::vector<std::pair<size_t, size_t>>> block_edges{};
    block_edges.clear();
    const auto partition_impl = [num_threads = par::num_threads(),
                                 positions = r[particles],
                                 parts = parinfo[particles],
                                 &adjacency,
                                 &secondary_partitioning_factory](
                                    this auto& self,
                                    const auto& partitioning_factory,
                                    auto&& indices,
                                    auto&& edges,
                                    size_t init_part) {
      // Build the partitioning for the current partitcles.
      [[maybe_unused]] const auto partition = partitioning_factory(
          std::views::all(indices) |
              std::views::transform([positions](size_t a) -> const auto& {
                return positions[a];
              }),
          std::views::all(indices) |
              std::views::transform([parts](size_t a) -> auto& { //
                return parts[a];
              }),
          /*num_parts=*/num_threads,
          init_part);

      // Prepare the block adjacency graph: one block for each partition,
      // plus one block for the edges that cross the partition boundaries.
      TIT_ASSERT(init_part <= block_edges.size(), "Invalid partition index!");
      block_edges.resize(
          std::max(block_edges.size(), init_part + num_threads + 1));

      // Collect the block adjacency graph for the current partition:
      // edges that are within the same partition go to the corresponding block,
      // edges that cross the partition boundaries go to the last block.
      for (const auto& [a, b] : edges) {
        if (const auto part = parts[a]; part == parts[b]) {
          block_edges[part].emplace_back(a, b);
        } else {
          block_edges.back().emplace_back(a, b);
        }
      }

      // Are we done?
      if (const auto final_pass = init_part > 0; final_pass) return;

      // Collect particles for the next partitioning pass: that are the
      // particles with edges crossing the partition boundaries.
      std::vector<size_t> next_indices;
      for (const size_t a : indices) {
        for (const size_t b : adjacency[a]) {
          if (parts[a] != parts[b] && parts[b] >= init_part) {
            next_indices.push_back(a);
            break;
          }
        }
      }

      // Collect edges for the next partitioning pass.
      const auto next_edges = std::move(block_edges.back());
      block_edges.pop_back();

      // Recurse into the next partitioning pass.
      self(secondary_partitioning_factory,
           next_indices,
           next_edges,
           init_part + num_threads);
    };
    partition_impl(primary_partitioning_factory_,
                   std::views::iota(size_t{0}, particles.size()),
                   adjacency_.edges(),
                   /*init_part=*/0);

    // Compress the block adjacency graph.
    block_edges_.clear();
    for (const auto& x : block_edges) block_edges_.push_back(x);

    // Report the block sizes.
    TIT_STATS("ParticleMesh::block_edges_", block_edges_.sizes());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  graph::Graph adjacency_;
  graph::Graph interp_adjacency_;
  Multivector<std::pair<size_t, size_t>> block_edges_;
  [[no_unique_address]] SearchFactory search_factory_;
  [[no_unique_address]] PrimaryPartitioningFactory
      primary_partitioning_factory_;
  [[no_unique_address]] SecondaryPartitioningFactory
      secondary_partitioning_factory_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
