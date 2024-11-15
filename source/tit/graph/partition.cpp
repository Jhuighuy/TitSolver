/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <iterator>
#include <map>
#include <ranges>
#include <set>
#include <unordered_map>
#include <vector>

#include <metis.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/multivector.hpp"
#include "tit/core/profiler.hpp"

#include "tit/graph/coarsen.hpp"
#include "tit/graph/graph.hpp"
#include "tit/graph/partition.hpp"
#include "tit/graph/refine.hpp"
#include "tit/graph/simple_partition.hpp" // IWYU pragma: keep

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

/// Multilevel graph partitioning.
///
/// The algorithm recursively coarsens the graph, partitions the coarsest graph,
/// and then refines the partitioning by moving nodes between partitions.
///
/// @param[in]  graph     Graph.
/// @param[in]  weights   Node weights.
/// @param[out] parts     Node partitioning.
/// @param[in]  num_parts Number of partitions.
/// @param[in]  max_depth Maximum number of coarsening iterations.
/// @param[in]  max_iter  Maximum number of refinement iterations.
void partition_multilevel(const WeightedGraph& graph,
                          const std::vector<weight_t>& weights,
                          std::vector<size_t>& parts,
                          size_t num_parts,
                          size_t max_depth,
                          size_t max_iter) {
  TIT_PROFILE_SECTION("Graph::partition_multilevel()");

  RefinePartsFM refine{};

  // Multilevel partitioning recursive implementation.
  const auto impl = [max_depth, num_parts, max_iter, &refine]( // NOLINT
                        this const auto& self,
                        size_t depth,
                        const WeightedGraph& fine_graph,
                        const std::vector<weight_t>& fine_weights,
                        std::vector<size_t>& fine_parts) -> void {
    TIT_ASSERT(fine_graph.num_nodes() == fine_weights.size(),
               "Invalid fine graph weights!");
    TIT_ASSERT(fine_graph.num_nodes() == fine_parts.size(),
               "Invalid fine graph parts!");

    // Coarsen the graph.
    const CoarsenHEM coarsen{};
    WeightedGraph coarse_graph{};
    std::vector<weight_t> coarse_weights{};
    std::vector<node_t> coarse_to_fine{};
    std::vector<node_t> fine_to_coarse{};
    coarsen(fine_graph,
            fine_weights,
            coarse_graph,
            coarse_weights,
            coarse_to_fine,
            fine_to_coarse);

    // Should we stop coarsening?
    //
    // Coarsening is stopped when the number of coarse nodes is less than
    // `C * num_parts`, where (`C = 15`, suggested by Metis), or when the
    // node reduction from coarsening is less than 80%. Or simply if the
    // maximum coarsening depth is reached.
    const auto stop_coarsening =
        (depth >= max_depth) || //
        (coarse_graph.num_nodes() <= 15 * num_parts) ||
        (coarse_graph.num_nodes() * 10 >= fine_graph.num_nodes() * 8);

    // Partition the coarse graph:
    std::vector<size_t> coarse_parts(coarse_graph.num_nodes());
    if (stop_coarsening) {
      // either partition the coarse graph directly (using Metis), ...
      SimplePartition{}(coarse_graph, coarse_weights, coarse_parts, num_parts);
      // partition_metis(coarse_graph, coarse_weights, coarse_parts, num_parts);
      // refiner(coarse_graph, coarse_weights, coarse_parts, num_parts);
    } else {
      // ... or coarsen the graph further.
      self(depth + 1, coarse_graph, coarse_weights, coarse_parts);
    }

    // Project the partitioning back to the fine graph.
    for (const auto fine_node : fine_graph.nodes()) {
      fine_parts[fine_node] = coarse_parts[fine_to_coarse[fine_node]];
    }

    // Refine the partitioning.
    refine(fine_graph, fine_weights, fine_parts, num_parts);
  };

  // Run the multilevel partitioning.
  impl(/*depth=*/0, graph, weights, parts);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
