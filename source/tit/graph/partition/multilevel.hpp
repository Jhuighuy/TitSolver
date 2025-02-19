/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/range_utils.hpp"

#include "tit/core/utils.hpp"
#include "tit/graph/coarsen.hpp"
#include "tit/graph/graph.hpp"
#include "tit/graph/partition/greedy.hpp"
#include "tit/graph/refine.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Multilevel graph partitioning function.
///
/// The algorithm recursively coarsens the graph, partitions the coarsest graph,
/// and then refines the partitioning by moving nodes between partitions.
template<class PartitionCoarsest = GreedyPartition,
         coarsen_func Coarsen = CoarsenGEM,
         refine_func Refine = RefinePartsFM>
class MultilevelPartition final {
public:

  /// Construct the multilevel partitioning function.
  /// @{
  constexpr MultilevelPartition() = default;
  constexpr explicit MultilevelPartition(PartitionCoarsest partition_coarsest,
                                         Coarsen coarsen,
                                         Refine refine) noexcept
      : partition_coarsest_{std::move(partition_coarsest)},
        coarsen_{std::move(coarsen)}, refine_{std::move(refine)} {}
  /// @}

  /// Partition the graph recursively using the multilevel partitioning
  /// algorithm.
  template<weighted_graph Graph, node_parts<Graph> Parts>
  void operator()(const Graph& graph, Parts&& parts, size_t num_parts) const {
    TIT_PROFILE_SECTION("Graph::MultilevelPartition::operator()");
    TIT_ASSUME_UNIVERSAL(Parts, parts);

    // Validate the arguments.
    TIT_ASSERT(num_parts > 0, "Number of parts must be positive!");
    TIT_ASSERT(num_parts <= graph.num_nodes(),
               "Number of nodes cannot be less than the number of parts!");
    if constexpr (std::ranges::sized_range<Parts>) {
      TIT_ASSERT(std::size(parts) == graph.num_nodes(),
                 "Size of parts range must be equal to the number of nodes!");
    }

    // Recursively partition the graph level by level.
    [&coarsen = coarsen_,
     &refine = refine_,
     &partition_coarsest = partition_coarsest_,
     num_parts](this const auto& self,
                const weighted_graph auto& fine_graph,
                std::ranges::view auto fine_parts) -> void {
      TIT_ASSERT(fine_graph.num_nodes() == std::size(fine_parts),
                 "Invalid fine graph parts!");

      // Coarsen the graph.
      tit::graph::WeightedGraph coarse_graph{};
      std::vector<node_t> coarse_to_fine{};
      std::vector<node_t> fine_to_coarse{};
      coarsen(fine_graph, coarse_graph, coarse_to_fine, fine_to_coarse);

      // Should we stop coarsening?
      //
      // Coarsening is stopped when the number of coarse nodes is less than
      // `C * num_parts`, where (`C = 15`, suggested by Metis), or when the
      // node reduction from coarsening is less than 80%.
      const auto stop_coarsening =
          (coarse_graph.num_nodes() <= 15 * num_parts) ||
          (coarse_graph.num_nodes() * 10 >= fine_graph.num_nodes() * 8);

      // Partition the coarse graph:
      std::vector<part_t> coarse_parts(coarse_graph.num_nodes());
      if (stop_coarsening) {
        // either partition the coarse graph directly, ...
        partition_coarsest(coarse_graph, coarse_parts, num_parts);
      } else {
        // ... or coarsen the graph further.
        self(coarse_graph, std::views::all(coarse_parts));
      }

      // Project the partitioning back to the fine graph and refine it.
      std::ranges::copy(permuted_view(coarse_parts, fine_to_coarse),
                        std::begin(fine_parts));
      refine(fine_graph, fine_parts, num_parts);
    }(graph, std::views::all(parts));
  }

private:

  [[no_unique_address]] PartitionCoarsest partition_coarsest_;
  [[no_unique_address]] Coarsen coarsen_;
  [[no_unique_address]] Refine refine_;

}; // class MultilevelPartition

/// Multilevel graph partitioning.
inline constexpr MultilevelPartition multilevel_partition{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
