/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

#include <metis.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/partition.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning using the Metis library.
void partition_metis(const WeightedGraph& graph,
                     const std::vector<weight_t>& weights,
                     std::vector<size_t>& parts,
                     size_t num_parts) {
  TIT_PROFILE_SECTION("Graph::partition_metis()");
  TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
  TIT_ENSURE(num_parts <= graph.num_nodes(), "Insufficient number of nodes!");

  // Setup the basic graph structure.
  idx_t ncon = 1;
  auto nvtxs = static_cast<idx_t>(graph.num_nodes());
  auto nparts = static_cast<idx_t>(num_parts);

  // Copy the graph structure into the internal format.
  std::vector<idx_t> xadj{0};
  std::vector<idx_t> adjncy;
  std::vector<idx_t> vwgt;
  std::vector<idx_t> adjwgt;
  for (const auto node : graph.nodes()) {
    std::ranges::copy(graph[node] | std::views::keys,
                      std::back_inserter(adjncy));
    xadj.emplace_back(adjncy.size());
    vwgt.emplace_back(weights[node]);
    std::ranges::copy(graph[node] | std::views::values,
                      std::back_inserter(adjwgt));
  }

  // Partition the graph.
  idx_t edgecut = 0;
  std::vector<idx_t> part(graph.num_nodes());
  const auto result = METIS_PartGraphKway(
      /*  idx_t* nvtxs   = */ &nvtxs,
      /*  idx_t* ncon    = */ &ncon,
      /*  idx_t* xadj    = */ xadj.data(),
      /*  idx_t* adjncy  = */ adjncy.data(),
      /*  idx_t* vwgt    = */ vwgt.data(),
      /*  idx_t* vsize   = */ nullptr,
      /*  idx_t* adjwgt  = */ adjwgt.data(),
      /*  idx_t* nparts  = */ &nparts,
      /* real_t* tpwgts  = */ nullptr,
      /* real_t* ubvec   = */ nullptr,
      /*  idx_t* options = */ nullptr,
      /*  idx_t* edgecut = */ &edgecut,
      /*  idx_t* part    = */ part.data());
  switch (result) {
    case METIS_OK:           break;
    case METIS_ERROR_INPUT:  TIT_FAIL("Metis: erroneous inputs!");
    case METIS_ERROR_MEMORY: TIT_FAIL("Metis: insufficient memory!");
    case METIS_ERROR:        TIT_FAIL("Metis: failure!");
    default:                 TIT_FAIL("Metis: unknown error!");
  }

  // Copy the partitioning.
  std::ranges::copy(part, std::begin(parts));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
