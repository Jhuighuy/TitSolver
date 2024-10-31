/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/multivector.hpp"
#include "tit/core/math.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/grid.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid spatial search index.
template<point_range Points>
  requires std::ranges::view<Points>
class GridIndex final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  /// Index the points for search using a grid.
  ///
  /// @param size_hint Cell size hint, typically 2x of the particle spacing.
  GridIndex(Points points, vec_num_t<Vec> size_hint)
      : points_{std::move(points)} {
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");

    // Compute bounding box and initialize the grid.
    const auto box = compute_bbox(points_).grow(size_hint / 2);
    // NOLINTNEXTLINE(*-prefer-member-initializer)
    grid_ = Grid{box}.set_cell_extents(size_hint);

    // Pack the points into a multivector.
    cell_points_.assign_pairs_par_tall(
        grid_.flat_num_cells(),
        iota_perm(points_) | std::views::transform([this](size_t point) {
          const auto cell = grid_.flat_cell_index(points_[point]);
          return std::pair{cell, point};
        }));
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter,
           std::predicate<size_t> Pred = AlwaysTrue>
  auto search(const Vec& search_point,
              vec_num_t<Vec> search_radius,
              OutIter out,
              Pred pred = {}) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");

    // Calculate the search box.
    const auto search_box = BBox{search_point}.grow(search_radius);

    // Collect points within the search box.
    const auto search_dist = pow2(search_radius);
    for (const auto& cell_index : grid_.cells_intersecting(search_box)) {
      const auto flat_cell_index = grid_.flatten_cell_index(cell_index);
      out = copy_points_near(points_,
                             cell_points_[flat_cell_index],
                             out,
                             search_point,
                             search_dist,
                             pred);
    }

    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  Points points_;
  Grid<Vec> grid_;
  Multivector<size_t> cell_points_;

}; // class GridIndex

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
GridIndex(Points&&, Args...) -> GridIndex<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Grid based spatial search indexing function.
class GridSearch final {
public:

  /// Construct a grid search indexing function.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridSearch(real_t size_hint) : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Index the points for search using a grid.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<GridIndex, Points&&, real_t>
  [[nodiscard]] auto operator()(Points&& points) const {
    TIT_PROFILE_SECTION("GridSearch::operator()");
    return GridIndex{std::forward<Points>(points), size_hint_};
  }

private:

  real_t size_hint_;

}; // class GridSearch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
