/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/par/atomic.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid spatial search index.
template<point_range Points>
  requires std::ranges::view<Points>
class GridIndex final {
public:

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Index the points for search using a grid.
  ///
  /// @param size_hint Cell size hint, typically 2x of the particle spacing.
  GridIndex(Points points, vec_num_t<Vec> size_hint)
      : points_{std::move(points)} {
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");

    // Early exit if the no points are provided.
    if (std::ranges::empty(points_)) return;
    const auto point_indices =
        std::views::iota(std::size_t{0}, std::ranges::size(points_));

    // Compute the bounding box.
    const auto box = compute_bbox(points_).grow(size_hint / 2);
    grid_ = Grid{box}.set_cell_extents(size_hint);

    // Count the points in each cell. Counts are shifted by two so that the
    // fill positions turn into the final cell offsets in-place.
    cell_point_offsets_.resize(grid_.flat_num_cells() + 2);
    par::for_each(point_indices, [this](std::size_t point_index) {
      const auto flat_cell = grid_.flat_cell_index(points_[point_index]);
      TIT_ASSERT(flat_cell < grid_.flat_num_cells(),
                 "Cell index is out of range!");
      par::fetch_and_add(cell_point_offsets_[flat_cell + 2], 1);
    });

    // Convert the cell counts to offsets and allocate the point indices.
    std::partial_sum(cell_point_offsets_.begin() + 2,
                     cell_point_offsets_.end(),
                     cell_point_offsets_.begin() + 2);
    cell_points_.resize(cell_point_offsets_.back());

    // Fill each cell range in parallel. Incrementing the shifted offsets
    // leaves the final CSR offsets in the first `num_cells + 1` entries.
    par::for_each(point_indices, [this](std::size_t point_index) {
      const auto flat_cell = grid_.flat_cell_index(points_[point_index]);
      TIT_ASSERT(flat_cell < grid_.flat_num_cells(),
                 "Cell index is out of range!");
      const auto position =
          par::fetch_and_add(cell_point_offsets_[flat_cell + 1], 1);
      cell_points_[position] = point_index;
    });
    cell_point_offsets_.pop_back();
  }

  /// Find the points within the given sphere.
  template<std::output_iterator<std::size_t> OutIter>
  auto search(const BSphere<Vec>& search_sphere, OutIter out) const -> OutIter {
    if (std::ranges::empty(points_)) return out;
    for (const auto& cell : grid_.cells_intersecting(search_sphere.box())) {
      const auto flat_cell = grid_.flatten_cell_index(cell);
      const auto first = cell_point_offsets_[flat_cell];
      const auto last = cell_point_offsets_[flat_cell + 1];
      for (const auto point_index :
           std::ranges::subrange(std::next(cell_points_.begin(), first),
                                 std::next(cell_points_.begin(), last))) {
        if (search_sphere.contains(points_[point_index])) *out++ = point_index;
      }
    }
    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Points points_;
  Grid<Vec> grid_;
  std::vector<std::size_t> cell_point_offsets_;
  std::vector<std::size_t> cell_points_;

}; // class GridIndex

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
GridIndex(Points&&, Args...) -> GridIndex<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Grid based spatial search indexing function.
template<class Num>
class GridSearch final {
public:

  /// Construct a grid search indexing function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridSearch(Num size_hint) : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Index the points for search using a grid.
  template<std::ranges::viewable_range Points>
    requires point_range<Points> &&
             std::same_as<point_range_num_t<Points>, Num> &&
             deduce_constructible_from<GridIndex, Points&&, Num>
  [[nodiscard]] auto operator()(Points&& points) const {
    TIT_PROFILE_SECTION("GridSearch::operator()");
    return GridIndex{std::forward<Points>(points), size_hint_};
  }

private:

  Num size_hint_;

}; // class GridSearch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
