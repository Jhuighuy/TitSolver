/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <oneapi/tbb/concurrent_vector.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/io.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/inertial_bisection.hpp"
#include "tit/geom/metis.hpp"
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

struct Subgraph {
  std::vector<size_t> indices; // NOLINT
  Graph adjacency;             // NOLINT

  template<class Range>
  constexpr void push_back(size_t i, Range&& range) {
    indices.push_back(i);
    adjacency.push_back(std::forward<Range>(range));
  }

  constexpr auto operator[](size_t i) const noexcept {
    return adjacency[i] | std::views::transform([x = indices[i]](size_t j) {
             return std::tuple{x, j};
           });
  }

  constexpr auto size() const noexcept {
    return indices.size();
  }

  constexpr auto edges() const noexcept {
    return std::views::iota(size_t{0}, size()) |
           std::views::transform([this](size_t i) {
             return (*this)[i] | std::views::take_while([](auto xy) {
                      return std::get<1>(xy) < std::get<0>(xy);
                    });
           }) |
           std::views::join;
  }

  constexpr auto as_graph() const {
    Graph graph{};
    for (size_t i = 0; i < size(); ++i) {
      graph.push_back(                                         //
          adjacency[i] | std::views::transform([&](size_t j) { //
            // Binary-search for j in indices.
            const auto it = std::ranges::lower_bound(indices, j);
            TIT_ENSURE(it != indices.end(), "Index is out of range!");
            return it - indices.begin();
          }));
    }
    return graph;
  }
};

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
      fixed_.clear();
      fixed_.reserve(particles.size() / 2);
      static std::vector<std::vector<size_t>> adj_vov{};
      adj_vov.resize(particles.size());
      par::for_each(particles.all(), [&radius_func, &engine, this](PV a) {
        const auto search_point = r[a];
        const auto search_radius = radius_func(a);
        TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");
        auto& search_results = adj_vov[a.index()];
        search_results.clear();
        engine.search(search_point,
                      search_radius,
                      std::back_inserter(search_results));
        if (fixed[a]) *fixed_.grow_by(1) = a.index();
      });
      adjacency_.clear();
      for (const auto& adj : adj_vov) adjacency_.push_back(adj);
      adjacency_.sort();
      // -----------------------------------------------------------------------
      static std::vector<std::vector<size_t>> interp_adj_vov{};
      interp_adj_vov.resize(fixed_.size());
      par::for_each(
          _fixed(particles),
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
              return fixed[particles[b_index]];
            });
          });
      interp_adjacency_.clear();
      for (const auto& adj : interp_adj_vov) interp_adjacency_.push_back(adj);
      interp_adjacency_.sort();
    }
    // -------------------------------------------------------------------------
    // STEP II: partitioning.
    block_adjacency_.clear();
    depth = 0;
    build_blocks<true>(particles,
                       std::views::iota(0UZ, particles.size()),
                       adjacency_);
    block_edges_.clear();
    for (auto& block : block_adjacency_) {
      block_edges_.push_back(block.edges());
    }
    // -------------------------------------------------------------------------
    // STEP III: assembly.
    {
      // Finalize color ranges.
      eprint("NCOL: ");
      for (size_t i = 0; i < block_edges_.size(); ++i) {
        eprint("{} ", block_edges_[i].size());
      }
      eprint("\n");
    }
  }

  template<bool Root = false>
  constexpr void build_blocks(auto& particles,
                              auto&& indices,
                              auto&& adj) { // NOLINT
    const auto parts = particles[parinfo];
    const auto proj_parts =
        std::views::all(indices) |
        std::views::transform([parts](size_t i) -> auto& { return parts[i]; });
    const auto positions = particles[r];
    const auto proj_positions =
        std::views::all(indices) |
        std::views::transform(
            [positions](size_t i) -> const auto& { return positions[i]; });

    const auto init = block_adjacency_.size();
    const auto nparts = par::num_threads();
    const auto partitioner = [&] {
      if constexpr (Root) {
        return geom::InertialBisection{proj_positions,
                                       proj_parts,
                                       nparts,
                                       init};
      } else {
        const auto proj_graph = Subgraph{indices, adj}.as_graph();
        return geom::MetisPartitioner{proj_graph, proj_parts, nparts, init};
      }
    }();

    block_adjacency_.resize(init + nparts);
    Subgraph leftover_block{};
    par::for_each(std::views::iota(init, init + nparts + 1), [&](size_t part) {
      if (part != init + nparts) {
        for (const auto i : partitioner.part(part)) {
          const auto ia = indices[i];
          block_adjacency_[part].push_back(
              ia,
              adj[i] | std::views::take_while([&](size_t ib) {
                return ib < ia;
              }) | std::views::filter([&](size_t ib) {
                return parts[ib] == part;
              }));
        }
      } else {
        for (const auto [i, ia] : std::views::enumerate(indices)) {
          part = parts[ia];
          if (std::ranges::none_of(adj[i], [&](size_t ib) {
                return parts[ib] != part;
              }))
            continue;
          leftover_block.push_back(ia,
                                   adj[i] | std::views::filter([&](size_t ib) {
                                     return parts[ib] != part;
                                   }));
        }
      }
    });

    const auto too_deep = ++depth > 1;
    if (too_deep) {
      for (const auto ai : leftover_block.indices) parts[ai] = init + nparts;
      block_adjacency_.push_back(std::move(leftover_block));
      return;
    }

    build_blocks(particles, leftover_block.indices, leftover_block.adjacency);
  }

  template<class ParticleArray>
  constexpr auto _fixed(ParticleArray& particles) const noexcept {
    return std::views::iota(0UZ, fixed_.size()) |
           std::views::transform([&particles, this](size_t i) {
             return std::tuple{i, particles[fixed_[i]]};
           });
  }

  /// Adjacent particles.
  template<class PV>
  constexpr auto operator[](PV a) const noexcept {
    return std::views::all(adjacency_[a.index()]) |
           std::views::transform(
               [a](size_t b_index) { return a.array()[b_index]; });
  }

  /// Adjacent fixed particles.
  template<class ParticleArray>
  constexpr auto operator[](std::nullptr_t,
                            ParticleArray& particles,
                            size_t i) const noexcept {
    return std::views::all(interp_adjacency_[i]) |
           std::views::transform(
               [&](size_t b_index) { return particles[b_index]; });
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
    return std::views::iota(0UZ, block_edges_.size()) |
           std::views::transform([&](size_t block_index) {
             return block_edges_[block_index] |
                    std::views::transform([&particles](auto ab_indices) {
                      const auto [a_index, b_index] = ab_indices;
                      return std::tuple{particles[a_index], particles[b_index]};
                    });
           });
  }

private:

  EngineFactory engine_factory_;
  Graph adjacency_;
  tbb::concurrent_vector<size_t> fixed_;
  Graph interp_adjacency_;

  size_t depth = 0;
  std::vector<Subgraph> block_adjacency_;
  Multivector<std::tuple<size_t, size_t>> block_edges_;

}; // class ParticleMesh

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle mesh type.
template<class PM>
concept particle_mesh = specialization_of<PM, ParticleMesh>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
