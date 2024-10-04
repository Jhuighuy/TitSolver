/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/mdvector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/stats.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/partition.hpp"
#include "tit/graph/utils.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partitioning based on a graph partitioning of a grid cell connectivity.
template<std::ranges::view Points, std::ranges::view Parts>
  requires (std::ranges::sized_range<Points> &&
            std::ranges::random_access_range<Points> &&
            is_vec_v<std::ranges::range_value_t<Points>>) &&
           (std::ranges::random_access_range<Parts> &&
            std::ranges::output_range<Parts, size_t>)
class GraphBasedPartitioning final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  /// Initialize and build the partitioning.
  GraphBasedPartitioning(Points points,
                         Parts parts,
                         size_t num_parts,
                         size_t init_part = 0,
                         real_t size_hint = 2.0 * 0.6 / 80.0)
      : points_{std::move(points)}, parts_{std::move(parts)} {
    TIT_PROFILE_SECTION("GraphBasedPartitioning::GraphBasedPartitioning()");

    // Compute bounding box.
    //
    /// @todo Introduce a helper function for this computation.
    BBox box{points_[0]};
    for (const Vec& point : points_ | std::views::drop(1)) box.expand(point);
    box.grow(size_hint / 100);

    // Compute number of cells and cell sizes.
    const auto extents = box.extents();
    const auto num_cells_float = ceil(extents / size_hint);
    const auto cell_extents = extents / num_cells_float;
    const auto cell_extents_recip = Vec(1) / cell_extents;

    // We'll extend the number of cells by one in each direction to avoid the
    // conditionals near boundary. Grid origin is at lower left corner of the
    // extended bounding box.
    constexpr auto first_cell_index = vec_cast<size_t>(Vec(1));
    const auto num_cells = vec_cast<size_t>(num_cells_float + Vec(2));
    const auto origin = box.low() - cell_extents;

    // Project the points onto a grid: calculate number of points in each cell.
    struct CountAndIndex {
      size_t count = 0;
      size_t flat_index = std::numeric_limits<size_t>::max();
    };
    Mdvector<CountAndIndex, vec_dim_v<Vec>> cells{};
    std::apply([&cells](auto... es) { cells.assign(es...); },
               to_array(num_cells));
    const auto cell_at = [&cells](const auto& cell_index) -> CountAndIndex& {
      return std::apply(
          [&cells](auto... indices) -> CountAndIndex& {
            return cells[indices...];
          },
          to_array(cell_index));
    };
    par::for_each(points_, [&](const Vec& point) {
      const auto cell_index_float = (point - origin) * cell_extents_recip;
      const auto cell_index = vec_cast<size_t>(cell_index_float);
      auto& cell = cell_at(cell_index);
      par::fetch_and_add(cell.count, 1);
    });

    // Assign flat indices to the cells that have points.
    size_t cell_flat_index = 0;
    for (auto& cell : cells) {
      if (cell.count > 0) cell.flat_index = cell_flat_index++;
    }

    // Build the graph connecting the cells.
    using EdgeMap = boost::container::small_vector< //
        std::pair<size_t, graph::weight_t>,
        2 * vec_dim_v<Vec>>;
    graph::WeightedGraph graph;
    std::vector<graph::weight_t> node_weights;
    vec_md_for( //
        first_cell_index,
        num_cells - first_cell_index,
        [&](const auto& cell_index) {
          const auto& cell = cell_at(cell_index);
          if (cell.count == 0) return;

          EdgeMap edges;
          for (size_t d = 0; d < vec_dim_v<Vec>; ++d) {
            for (ssize_t i = -1; i <= 1; i += 2) {
              auto neighbor_cell_index = cell_index;
              neighbor_cell_index[d] += i;
              const auto& neighbor_cell = cell_at(neighbor_cell_index);
              if (neighbor_cell.count == 0) continue;

              const auto edge_weight = cell.count * neighbor_cell.count;
              edges.emplace_back(neighbor_cell.flat_index, edge_weight);
            }
          }

          std::ranges::sort(edges);
          graph.push_back(edges);
          node_weights.push_back(cell.count);
        });

    std::vector<size_t> graph_parts(graph.num_nodes());
    graph::partition_multilevel(graph, node_weights, graph_parts, num_parts);
    const auto ec = graph::edge_cut(graph, graph_parts);
    TIT_STATS("edge_cut", ec);

    par::for_each(std::views::enumerate(points_), [&](const auto& ip) {
      const auto& [i, point] = ip;
      const auto cell_index_float = (point - origin) * cell_extents_recip;
      const auto cell_index = vec_cast<size_t>(cell_index_float);
      const auto cell = cell_at(cell_index);
      parts_[i] = init_part + graph_parts[cell.flat_index];
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  using CellIndex_ = decltype(to_array(vec_cast<size_t>(std::declval<Vec>())));

  Points points_;
  Parts parts_;

}; // class GraphBasedPartitioning

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points,
         std::ranges::viewable_range Parts,
         class... Args>
GraphBasedPartitioning(Points&&, Parts&&, Args...)
    -> GraphBasedPartitioning<std::views::all_t<Points>,
                              std::views::all_t<Parts>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class GraphBasedPartitioningFactory {
public:

  template<std::ranges::viewable_range Points,
           std::ranges::viewable_range Parts>
    requires deduce_constructible_from<GraphBasedPartitioning,
                                       Points,
                                       Parts,
                                       size_t,
                                       size_t>
  constexpr auto operator()(Points&& points,
                            Parts&& parts,
                            size_t num_parts,
                            size_t init_part = 0) const {
    return GraphBasedPartitioning{std::forward<Points>(points),
                                  std::forward<Parts>(parts),
                                  num_parts,
                                  init_part};
  }

}; // class GraphBasedPartitioningFactory

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
