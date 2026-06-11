/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
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
    const auto box = compute_bbox(points_).grow(size_hint / 2);
    grid_ = Grid{box}.set_cell_extents(size_hint);
    first_point_.assign(grid_.flat_num_cells(), sentinel_);
    next_point_.resize(std::ranges::size(points_));
    par::for_each(std::views::enumerate(std::views::zip(points_, next_point_)),
                  [this](auto&& index_and_points) {
                    auto&& [index, point_and_next_point] = index_and_points;
                    auto&& [point, next_point] = point_and_next_point;
                    const auto cell = grid_.flat_cell_index(point);
                    next_point = par::exchange(first_point_[cell], index);
                  });
  }

  /// Find the points within the given sphere.
  template<std::output_iterator<std::size_t> OutIter>
  auto search(const BSphere<Vec>& search_sphere, OutIter out) const -> OutIter {
    for (const auto& cell : grid_.cells_intersecting(search_sphere.box())) {
      for (auto index = first_point_[grid_.flatten_cell_index(cell)];
           index != sentinel_;
           index = next_point_[index]) {
        if (search_sphere.contains(points_[index])) *out++ = index;
      }
    }
    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Points points_;
  Grid<Vec> grid_;
  std::vector<std::size_t> first_point_;
  std::vector<std::size_t> next_point_;
  static constexpr auto sentinel_ = std::numeric_limits<std::size_t>::max();

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
