/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <iterator>
#include <ranges>
#include <vector>

#include <metis.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/profiler.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Metis-based graph partitioner.
template<std::ranges::view Parts>
  requires (std::ranges::random_access_range<Parts> &&
            std::ranges::output_range<Parts, size_t>)
class MetisPartitioner final {
public:

  /// Initialize and build the partitioning.
  MetisPartitioner(const Graph& graph,
                   Parts parts,
                   size_t num_parts,
                   size_t init_part = 0)
      : parts_{std::move(parts)} {
    TIT_PROFILE_SECTION("MetisPartitioner::MetisPartitioner()");
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");

    // Setup the basic graph structure.
    idx_t ncon = 1;
    auto nvtxs = static_cast<idx_t>(graph.num_nodes());
    auto nparts = static_cast<idx_t>(num_parts);

    // Copy the graph structure into the internal format.
    std::vector<idx_t> xadj{0};
    std::vector<idx_t> adjncy;
    for (size_t i = 0; i < graph.num_nodes(); ++i) {
      std::ranges::copy(graph[i], std::back_inserter(adjncy));
      xadj.emplace_back(adjncy.size());
    }

    // Define the output arrays.
    idx_t edgecut = 0;
    std::vector<idx_t> part(graph.num_nodes());

    // Partition the graph.
    const auto result = METIS_PartGraphKway(
        /*idx_t* nvtxs=*/&nvtxs,
        /*idx_t* ncon=*/&ncon,
        /*idx_t* xadj=*/xadj.data(),
        /*idx_t* adjncy=*/adjncy.data(),
        /*idx_t* vwgt=*/nullptr,
        /*idx_t* vsize=*/nullptr,
        /*idx_t* adjwgt=*/nullptr,
        /*idx_t* nparts=*/&nparts,
        /*real_t* tpwgts=*/nullptr,
        /*real_t* ubvec=*/nullptr,
        /*idx_t* options=*/nullptr,
        /*idx_t* edgecut=*/&edgecut,
        /*idx_t* part=*/part.data());
    TIT_ENSURE(result == METIS_OK, "Metis errored!");

    // Copy the partitioning.
    std::ranges::copy(part | std::views::transform([init_part](idx_t p) {
                        return p + init_part;
                      }),
                      std::begin(parts_));
  }

  /// Get the part range.
  constexpr auto part(size_t part_index) const noexcept {
    return std::views::iota(size_t{0}, parts_.size()) |
           std::views::filter([part_index, this](size_t i) {
             return parts_[i] == part_index;
           });
  }

private:

  Parts parts_;

}; // class MetisPartitioner

// Wrap a viewable range into a view on construction.
template<class Parts, class... Args>
MetisPartitioner(const Graph&,
                 Parts&&,
                 Args...) -> MetisPartitioner<std::views::all_t<Parts>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
