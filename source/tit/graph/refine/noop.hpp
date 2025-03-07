/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/range.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No graph partition refinement.
class RefinePartsNoop final {
public:

  /// Refine the partitioning of the given graph.
  template<weighted_graph Graph, input_output_index_range Parts>
  static constexpr void operator()(const Graph& /*graph*/,
                                   Parts& /*parts*/,
                                   size_t /*num_parts*/) {
    // Nothing to do.
  }

}; // class RefinePartsNoop

/// No graph partition refinement.
inline constexpr RefinePartsNoop do_not_refine_parts{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
