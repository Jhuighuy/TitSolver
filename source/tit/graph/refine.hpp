/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partition refinement algorithm configuration.
struct RefinementConfig {
  /// Maximum allowed disbalance between partition weights in percent.
  weight_t max_disbalance = 3;

  /// Maximum number of iterations.
  size_t max_iter = 20;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fiduccia-Mattheyses-style graph partition refinement.
///
/// The graph partition refinement is used to refine the partitioning of the
/// given graph. The refinement is done by moving nodes between partitions to
/// minimize the edge cut while maintaining a balance between partition weights.
/// On each iteration, nodes are moved to partitions that maximize the gain
/// while keeping the balance. Negative gain moves are allowed, the solution is
/// rolled back to the best state achieved during the iteration.
class GraphPartsRefiner final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize the graph partition refiner.
  explicit GraphPartsRefiner(const RefinementConfig& config = {})
      : config_{config} {}

  /// Refine the partitioning of the given graph.
  void operator()(const WeightedGraph& graph,
                  const WeightArray& weights,
                  PartArray& parts,
                  size_t num_parts);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  RefinementConfig config_;

}; // class GraphPartsRefiner

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
