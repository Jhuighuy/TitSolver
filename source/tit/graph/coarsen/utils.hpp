/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/containers/small_flat_map.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Build the coarse graph from the fine graph and coarse-fine mapping.
template<weighted_graph FineGraph,
         weighted_graph CoarseGraph,
         index_range CoarseToFine,
         index_range FineToCoarse>
void build_coarse_graph(const FineGraph& fine_graph,
                        CoarseGraph& coarse_graph,
                        CoarseToFine&& coarse_to_fine,
                        FineToCoarse&& fine_to_coarse) {
  TIT_ASSUME_UNIVERSAL(CoarseToFine, coarse_to_fine);
  TIT_ASSUME_UNIVERSAL(FineToCoarse, fine_to_coarse);
  coarse_graph.clear();
  equality_ranges(
      coarse_to_fine,
      [&fine_graph, &fine_to_coarse, &coarse_graph](auto fine_nodes) {
        weight_t coarse_weight = 0;
        SmallFlatMap<node_t, weight_t, 32> coarse_neighbors{};
        for (const auto fine_node : fine_nodes) {
          coarse_weight += fine_graph.weight(fine_node);
          for (const auto& [fine_neighbor, fine_edge_weight] :
               fine_graph.wedges(fine_node)) {
            const auto coarse_neighbor = fine_to_coarse[fine_neighbor];
            coarse_neighbors[coarse_neighbor] += fine_edge_weight;
          }
        }
        coarse_graph.append_node(coarse_weight, coarse_neighbors);
      },
      /*pred=*/{},
      /*proj=*/[&fine_to_coarse](node_t fn) { return fine_to_coarse[fn]; });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph::impl
