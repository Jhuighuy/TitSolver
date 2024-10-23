/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "tit/core/basic_types.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning using the Metis library.
///
/// @param[in]  graph     Graph.
/// @param[in]  weights   Node weights.
/// @param[out] parts     Node partitioning.
/// @param[in]  num_parts Number of partitions.
void partition_metis(const WeightedGraph& graph,
                     const std::vector<weight_t>& weights,
                     std::vector<size_t>& parts,
                     size_t num_parts);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
