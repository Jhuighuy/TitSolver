/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/par/control.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle adjacency graph.
template<geom::search_func SearchFunc,
         geom::face_search_func FaceSearchFunc,
         geom::partition_func PartitionFunc,
         geom::partition_func InterfacePartitionFunc = PartitionFunc>
class ParticleMesh final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle adjacency graph.
  ///
  /// @param search_func Nearest-neighbors search indexing function.
  /// @param face_search_func Face search function.
  /// @param partition_func Geometry partitioning function.
  /// @param interface_partition_func Interface partitioning function.
  constexpr explicit ParticleMesh(
      SearchFunc search_func = {},
      FaceSearchFunc face_search_func = {},
      PartitionFunc partition_func = {},
      InterfacePartitionFunc interface_partition_func = {}) noexcept
      : search_func_{std::move(search_func)},
        face_search_func_{std::move(face_search_func)},
        partition_func_{std::move(partition_func)},
        interface_partition_func_{std::move(interface_partition_func)} {}

  /// Adjacent particles.
  template<particle_view PV>
  constexpr auto operator[](PV a) const noexcept {
    auto& particles = a.array();
    return adjacency_[a.index()] |
           std::views::transform(
               [&particles](std::size_t b) { return particles[b]; });
  }

  /// Adjacent faces.
  template<class Domain, particle_view PV>
  constexpr auto operator[](const Domain& domain, PV a) const noexcept {
    auto& particles = a.array();
    return face_adjacency_[a.index()] |
           std::views::transform([&domain, &particles](std::size_t face_index) {
             const auto& [... vert_indices] = domain.face_verts(face_index);
             return std::pair{domain.face(face_index),
                              std::tuple{particles.fixed()[vert_indices]...}};
           });
  }

  /// Unique pairs of the adjacent particles.
  template<particle_array ParticleArray>
  constexpr auto pairs(ParticleArray& particles) const noexcept {
    return adjacency_ | std::views::transform([&particles](auto ab) {
             const auto [a, b] = ab;
             return std::tuple{particles[a], particles[b]};
           });
  }

  /// Unique pairs of the adjacent particles partitioned by the block.
  template<particle_array ParticleArray>
  constexpr auto block_pairs(ParticleArray& particles) const noexcept {
    return std::views::all(block_edges_) |
           std::views::transform([&particles](const auto& block) {
             return block | std::views::transform([&particles](auto ab) {
                      const auto [a, b] = ab;
                      return std::tuple{particles[a], particles[b]};
                    });
           });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Update the adjacency graph.
  template<class Domain, particle_array ParticleArray, class SearchRadiusFunc>
  void update(const Domain& domain,
              ParticleArray& particles,
              const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleMesh::update()");

    // Update the adjacency graphs.
    search_(domain, particles, radius_func);

    // Partition the adjacency graph by the block.
    partition_(particles);
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

    // Search for the neighbors.
    adjacency_.resize(particles.size());
    par::for_each(particles.all(), [&radius_func, &search_index, this](PV a) {
      const auto& search_point = r[a];
      const auto search_radius = radius_func(a);
      TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");

      auto& search_results = adjacency_[a.index()];
      search_results.clear();
      search_index.search(geom::BSphere{search_point, search_radius},
                          std::back_inserter(search_results));
      std::ranges::sort(search_results);
    });

    // Search for the adjacent boundary faces.
    face_adjacency_.resize(particles.size());
    par::for_each(particles.all(), [&radius_func, &face_index, this](PV a) {
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

  template<particle_array ParticleArray>
  void partition_(const ParticleArray& particles, std::size_t num_levels = 2) {
    TIT_PROFILE_SECTION("ParticleMesh::partition()");
    TIT_ASSERT(num_levels > 0, "Number of levels must be positive!");
    TIT_ASSERT(num_levels < max_num_levels_,
               "Number of levels exceeds the predefined maximum!");

    // Initialize the partitioning.
    const auto num_threads = par::num_threads();
    const auto num_parts = num_levels * num_threads + 1;
    constexpr auto max_num_parts = std::numeric_limits<PartIndex_>::max();
    TIT_ENSURE(num_parts < max_num_parts,
               "Number of parts exceeded the limit of {}.",
               max_num_parts);
    std::vector parts(particles.size(),
                      PartVec_(static_cast<PartIndex_>(num_parts - 1)));

    // Build the multi-level partitioning.
    const auto positions = r[particles];
    std::vector<std::size_t> interface{};
    std::vector<std::size_t> prev_interface{};
    for (std::size_t level = 0; level < num_levels; ++level) {
      const auto is_first_level = level == 0;
      const auto is_last_level = level == (num_levels - 1);

      // Partition the particles.
      const auto level_parts =
          parts | std::views::transform(
                      [level](PartVec_& part) -> auto& { return part[level]; });
      if (is_first_level) {
        partition_func_(positions,
                        level_parts,
                        static_cast<PartIndex_>(num_threads));
      } else {
        interface_partition_func_(
            permuted_view(positions, interface),
            permuted_view(level_parts, interface),
            static_cast<PartIndex_>(num_threads),
            /*init_part=*/static_cast<PartIndex_>(level * num_threads));
      }
      if (is_last_level) break;

      // Update the interface particles.
      const auto is_interface = [level_parts, this](std::size_t a) {
        return std::ranges::any_of(
            permuted_view(level_parts, adjacency_[a]),
            std::bind_front(std::not_equal_to{}, level_parts[a]));
      };
      const auto update_interface = [&interface,
                                     &is_interface](const auto& current) {
        interface.resize(std::ranges::size(current));
        interface.erase(
            par::unstable_copy_if(current, interface.begin(), is_interface),
            interface.end());
      };
      if (is_first_level) {
        update_interface(std::views::iota(std::size_t{0}, particles.size()));
      } else {
        interface.swap(prev_interface);
        update_interface(prev_interface);
      }
    }

    // Assemble the block adjacency graph.
    block_edges_.resize(num_parts);
    for (auto& block : block_edges_) block.clear();
    for (const auto& [index_, neighbors] :
         std::views::enumerate(adjacency_) | std::views::as_const) {
      const auto index = static_cast<std::size_t>(index_);
      for (const auto neighbor : neighbors) {
        if (neighbor >= index) break;
        const auto level = find_true(parts[index] == parts[neighbor]);
        TIT_ASSERT(level >= 0, "No common partition index!");
        const auto part = parts[index][level];
        block_edges_[part].emplace_back(index, neighbor);
      }
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  static constexpr std::size_t max_num_levels_ = 8;

  using PartIndex_ = std::uint8_t;
  using PartVec_ = Vec<PartIndex_, max_num_levels_>;

  std::vector<std::vector<std::size_t>> adjacency_;
  std::vector<std::vector<std::pair<std::size_t, std::size_t>>> block_edges_;
  std::vector<std::vector<std::size_t>> face_adjacency_;
  [[no_unique_address]] SearchFunc search_func_;
  [[no_unique_address]] FaceSearchFunc face_search_func_;
  [[no_unique_address]] PartitionFunc partition_func_;
  [[no_unique_address]] InterfacePartitionFunc interface_partition_func_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
