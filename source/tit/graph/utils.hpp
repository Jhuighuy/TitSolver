/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compute the edge cut of a graph partitioning.
///
/// The edge cut is the sum of the weights of the edges that connect nodes from
/// different partitions.
template<graph Graph, index_range Parts>
constexpr auto edge_cut(const Graph& graph, Parts&& parts) noexcept
    -> weight_t {
  TIT_ASSUME_UNIVERSAL(Parts, parts);
  weight_t edge_cut = 0;
  for (const auto& [node, neighbor, edge_weight] : graph.wedges()) {
    if (parts[node] != parts[neighbor]) edge_cut += edge_weight;
  }
  return edge_cut * 2;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
