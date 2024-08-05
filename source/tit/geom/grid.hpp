/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Uniform multidimensional grid.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class Grid final {
public:

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize and build the multidimensional grid.
  ///
  /// @param size_hint Grid cell size hint, typically 2x of the particle
  ///                  spacing.
  constexpr explicit Grid(Points points, vec_num_t<Vec> size_hint)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("Grid::Grid()");
    TIT_ASSERT(size_hint > 0.0, "Cell size hint must be positive!");

    // Compute bounding box.
    //
    /// @todo Introduce a helper function for this computation.
    grid_box_ = BBox{points_[0]}; // NOLINT(*-prefer-member-initializer)
    for (const auto& p : points_ | std::views::drop(1)) grid_box_.expand(p);
    grid_box_.grow(size_hint / 2);

    // Compute number of cells and cell sizes.
    const auto extents = grid_box_.extents();
    const auto num_cells_float = ceil(extents / size_hint);
    num_cells_ = static_vec_cast<size_t>(num_cells_float);
    cell_extents_ = extents / num_cells_float;
    cell_extents_recip_ = Vec(1) / cell_extents_; // NOLINT(*member-initializer)

    // Pack the points into a multivector.
    cell_points_.assemble_tall( //
        prod(num_cells_),
        std::views::iota(size_t{0}, points_.size()),
        [this](size_t index) {
          return flatten_cell_index_(cell_index_(points_[index]));
        });
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(const Vec& search_point,
                        vec_num_t<Vec> search_radius,
                        OutIter out) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");

    // Calculate the search box.
    const auto half_cell_extents = cell_extents_ / 2;
    const auto search_box = BBox{search_point}
                                .grow(search_radius)
                                .grow(half_cell_extents)
                                .intersect(grid_box_)
                                .shrink(half_cell_extents);
    const auto low = cell_index_(search_box.low());
    const auto high = cell_index_(search_box.high());

    // Collect points within the search box.
    const auto collect_cell_points = [&out,
                                      &search_point,
                                      search_dist = pow2(search_radius),
                                      this](const CellIndex_& cell_index) {
      std::ranges::copy_if(
          cell_points_[flatten_cell_index_(cell_index)],
          out,
          [&search_point, search_dist](const Vec& point) {
            return norm2(search_point - point) < search_dist;
          },
          [this](size_t i) -> const Vec& { return points_[i]; });
    };
    if constexpr (vec_dim_v<Vec> == 1) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        collect_cell_points({i});
      }
    } else if constexpr (vec_dim_v<Vec> == 2) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          collect_cell_points({i, j});
        }
      }
    } else if constexpr (vec_dim_v<Vec> == 3) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          for (size_t k = low[2]; k <= high[2]; ++k) {
            collect_cell_points({i, j, k});
          }
        }
      }
    } else static_assert(false);

    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  using CellIndex_ = decltype(static_vec_cast<size_t>(std::declval<Vec>()));

  constexpr auto cell_index_(const Vec& point) const -> CellIndex_ {
    const auto index_float = (point - grid_box_.low()) * cell_extents_recip_;
    TIT_ASSERT(all(index_float >= Vec(0)), "Point is out of range!");
    return static_vec_cast<size_t>(index_float);
  }

  constexpr auto flatten_cell_index_(const CellIndex_& index) const noexcept
      -> size_t {
    auto flat_index = index[0];
    for (size_t i = 1; i < vec_dim_v<Vec>; ++i) {
      flat_index = num_cells_[i] * flat_index + index[i];
    }
    return flat_index;
  }

  Points points_;
  BBox<Vec> grid_box_;
  CellIndex_ num_cells_;
  Vec cell_extents_;
  Vec cell_extents_recip_;
  Multivector<size_t> cell_points_;

}; // class Grid

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
Grid(Points&&, Args...) -> Grid<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Multidimensional grid factory.
class GridFactory final {
public:

  /// Construct a multidimensional grid factory.
  ///
  /// @param size_hint Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridFactory(real_t size_hint) : size_hint_{size_hint} {
    TIT_ASSERT(size_hint_ > 0.0, "Cell size hint must be positive!");
  }

  /// Produce a multidimensional grid for the specified set of points.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<Grid, Points&&, real_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return Grid{std::forward<Points>(points), size_hint_};
  }

private:

  real_t size_hint_;

}; // class GridFactory

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
