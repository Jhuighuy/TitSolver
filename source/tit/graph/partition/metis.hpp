/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <string_view>
#include <vector>

#include <metis.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range.hpp"
#include "tit/core/utils.hpp"

#include "tit/graph/graph.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph partitioning using the METIS library.
class MetisPartition final {
public:

  template<weighted_graph Graph, output_index_range Parts>
  void operator()(const Graph& graph, Parts&& parts, size_t num_parts) const {
    TIT_PROFILE_SECTION("Graph::MetisPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(num_parts <= graph.num_nodes(),
               "Number of nodes cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(parts) == graph.num_nodes(),
                 "Size of parts range must be equal to the number of nodes!");
    }

    // Setup the input.
    idx_t ncon = 1; // number of balancing constraints.
    auto nvtxs = static_cast<idx_t>(graph.num_nodes());
    auto nparts = static_cast<idx_t>(num_parts);
    adjncy_.clear(), adjwgt_.clear();
    xadj_.reserve(graph.num_nodes() + 1), xadj_.clear(), xadj_.push_back(0);
    vwgt_.reserve(graph.num_nodes()), vwgt_.clear();
    for (const auto& [node, weight] : graph.wnodes()) {
      for (const auto& [neighbor, edge_weight] : graph.wedges(node)) {
        adjncy_.push_back(neighbor);
        adjwgt_.push_back(static_cast<idx_t>(edge_weight));
      }
      xadj_.push_back(static_cast<idx_t>(adjncy_.size()));
      vwgt_.push_back(static_cast<idx_t>(weight));
    }

    // Setup the output.
    idx_t edgecut = 0;
    std::vector<idx_t> part(graph.num_nodes());

    // Partition the graph.
    if (const auto status = METIS_PartGraphKway(
            /* idx_t*  nvtxs   = */ &nvtxs,
            /* idx_t*  ncon    = */ &ncon,
            /* idx_t*  xadj    = */ xadj_.data(),
            /* idx_t*  adjncy  = */ adjncy_.data(),
            /* idx_t*  vwgt    = */ vwgt_.data(),
            /* idx_t*  vsize   = */ nullptr,
            /* idx_t*  adjwgt  = */ adjwgt_.data(),
            /* idx_t*  nparts  = */ &nparts,
            /* real_t* tpwgts  = */ nullptr,
            /* real_t* ubvec   = */ nullptr,
            /* idx_t*  options = */ nullptr,
            /* idx_t*  edgecut = */ &edgecut,
            /* idx_t*  part    = */ part.data());
        status != METIS_OK) {
      const auto what = translate<std::string_view>(status)
                            .option(METIS_ERROR_INPUT, "input error.")
                            .option(METIS_ERROR_MEMORY, "out of memory.")
                            .fallback("unknown error.");
      TIT_THROW("`METIS_PartGraphKway` failed: {} ({}).", what, status);
    }

    // Copy the partitioning to the output.
    std::ranges::copy(part, std::begin(parts));
  }

private:

  mutable std::vector<idx_t> xadj_;
  mutable std::vector<idx_t> adjncy_;
  mutable std::vector<idx_t> vwgt_;
  mutable std::vector<idx_t> adjwgt_;

}; // class MetisPartition

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
