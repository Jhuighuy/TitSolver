/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <concepts>
#include <span>

#include <metis.h>

#include "tit/core/checks.hpp"

#include "tit/graph/partition/metis.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void MetisPartition::run_metis_(std::span<metis_index_t_> node_weights,
                                std::span<metis_index_t_> edge_ranges,
                                std::span<metis_index_t_> edge_neighbors,
                                std::span<metis_index_t_> edge_weights,
                                std::span<metis_index_t_> parts,
                                metis_index_t_ num_parts) {
  static_assert(std::same_as<metis_index_t_, idx_t>, "Index type mismatch!");
  TIT_ASSERT(node_weights.size() + 1 == edge_ranges.size(), "Size mismatch!");
  TIT_ASSERT(edge_neighbors.size() == edge_weights.size(), "Size mismatch!");
  TIT_ASSERT(parts.size() == node_weights.size(), "Size mismatch!");
  TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
  TIT_ASSERT(num_parts <= to_metis_(node_weights.size()),
             "Number of nodes cannot be less than the number of parts!");

  // Partition the graph.
  idx_t edgecut = 0;
  auto num_nodes = to_metis_(node_weights.size());
  auto num_conns = to_metis_(1); // I do not know what this is for.
  const auto result = METIS_PartGraphKway(
      /*  idx_t* nvtxs   = */ &num_nodes,
      /*  idx_t* ncon    = */ &num_conns,
      /*  idx_t* xadj    = */ edge_ranges.data(),
      /*  idx_t* adjncy  = */ edge_neighbors.data(),
      /*  idx_t* vwgt    = */ node_weights.data(),
      /*  idx_t* vsize   = */ nullptr,
      /*  idx_t* adjwgt  = */ edge_weights.data(),
      /*  idx_t* nparts  = */ &num_parts,
      /* real_t* tpwgts  = */ nullptr,
      /* real_t* ubvec   = */ nullptr,
      /*  idx_t* options = */ nullptr,
      /*  idx_t* edgecut = */ &edgecut,
      /*  idx_t* part    = */ parts.data());

  // Check the result.
  switch (result) {
    case METIS_OK:           break;
    case METIS_ERROR_INPUT:  TIT_FAIL("Metis: erroneous inputs!");
    case METIS_ERROR_MEMORY: TIT_FAIL("Metis: insufficient memory!");
    case METIS_ERROR:        TIT_FAIL("Metis: failure!");
    default:                 TIT_FAIL("Metis: unknown error!");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
