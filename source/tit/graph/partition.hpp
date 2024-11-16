/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/stats.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/simple_partition.hpp"
#include "tit/graph/utils.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning using the Metis library.
///
/// @param[in]  graph     Graph.
/// @param[in]  weights   Node weights.
/// @param[out] parts     Node partitioning.
/// @param[in]  num_parts Number of partitions.
void partition_metis(const WeightedGraph& graph,
                     std::vector<size_t>& parts,
                     size_t num_parts);

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
                          std::vector<size_t>& parts,
                          size_t num_parts,
                          size_t max_depth = 10,
                          size_t max_iter = 10);

struct MultilevelPartition {
  static void operator()(const auto& graph,
                         const auto& weights,
                         auto& parts,
                         size_t num_parts) {
    graph::WeightedGraph swg;
    for (const auto& edges : graph.buckets()) {
      swg.append_node(edges);
    }
    partition_multilevel(swg, weights, parts, num_parts);
    TIT_STATS("edge_cut", edge_cut(swg, parts));
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partition function type.
template<class PF>
concept partition_func = std::same_as<PF, UniformPartition> || //
                         std::same_as<PF, SimplePartition> ||  //
                         std::same_as<PF, MultilevelPartition>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
