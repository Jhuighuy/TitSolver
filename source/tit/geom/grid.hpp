/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <iterator>
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/profiler.hpp"
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
  using Point = std::ranges::range_value_t<Points>;

  /// Bounding box type.
  using PointBBox = BBox<Point>;

  /// Numeric type used by the point type.
  using Real = vec_num_t<Point>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<Point>;

private:

  Points points_;
  PointBBox grid_bbox_;
  Vec<size_t, Dim> num_cells_;
  Point cell_size_;
  Multivector<size_t> cell_points_;

public:

  /// Initialize and build the multidimensional grid.
  ///
  /// @param spacing Grid cell size, typically 2x of the particle spacing.
  constexpr explicit Grid(Points points, Real spacing)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("Grid::Grid()");
    TIT_ASSERT(spacing > 0.0, "Spacing must be positive.");
    if (std::ranges::empty(points_)) return;
    // Build the grid.
    _build_grid(spacing);
  }

private:

  // Compute index of the point.
  constexpr auto _point_to_cell_mdindex(Point point) const noexcept {
    point = (point - grid_bbox_.low()) / cell_size_;
    return static_vec_cast<size_t>(point);
  }
  constexpr auto _point_to_cell_index(Point point) const noexcept -> size_t {
    const auto mdindex = _point_to_cell_mdindex(point);
    auto index = mdindex[0];
    for (size_t i = 1; i < Dim; ++i) {
      index = num_cells_[i] * index + mdindex[i];
    }
    return index;
  }

  constexpr void _build_grid(Real spacing) {
    // Compute grid bounding box.
    grid_bbox_ = BBox{points_[0]};
    for (const auto& p : points_ | std::views::drop(1)) grid_bbox_.expand(p);
    grid_bbox_.grow(0.5 * spacing);
    // Compute number of cells and cell sizes.
    const auto extents = grid_bbox_.extents();
    const auto approx_num_cells = ceil(extents / spacing);
    num_cells_ = static_vec_cast<size_t>(approx_num_cells);
    cell_size_ = extents / approx_num_cells;
    auto total_num_cells = 1UZ;
    for (size_t i = 0; i < Dim; ++i) total_num_cells *= num_cells_[i];
    // Pack the points into a multivector.
    cell_points_.assemble_tall(
        total_num_cells,
        std::views::iota(size_t{0}, points_.size()),
        [this](size_t index) { return _point_to_cell_index(points_[index]); });
  }

public:

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Point search_point,
                        Real search_radius,
                        OutIter out) const noexcept -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    const auto search_dist = pow2(search_radius);
    // Convert point to bounding box.
    const auto search_bbox =
        BBox{search_point - Point(search_radius) - 0.5 * cell_size_,
             search_point + Point(search_radius) + 0.5 * cell_size_};
    const auto low = _point_to_cell_mdindex(
        grid_bbox_.clamp(search_bbox.low()) + 0.5 * cell_size_);
    const auto high = _point_to_cell_mdindex(
        grid_bbox_.clamp(search_bbox.high()) - 0.5 * cell_size_);
    for (size_t i = low[0]; i <= high[0]; ++i) {
      for (size_t j = low[1]; j <= high[1]; ++j) {
        const size_t cell_index = i * num_cells_[1] + j;
        for (const size_t k : cell_points_[cell_index]) {
          const auto dist = norm2(search_point - points_[k]);
          if (dist < search_dist) *out++ = k;
        }
      }
    }
    return out;
  }

}; // class Grid

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
Grid(Points&&, Args...) -> Grid<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class... Args>
concept can_grid = requires(Args... args) { Grid{args...}; };
} // namespace impl

/// Multidimensional grid factory.
class GridFactory final {
public:

  /// Construct a multidimensional grid factory.
  ///
  /// @param spacing Grid cell size, typically 2x of the particle spacing.
  constexpr explicit GridFactory(real_t spacing) : spacing_{spacing} {
    TIT_ASSERT(spacing_ > 0.0, "Spacing must be positive.");
  }

  /// Produce a multidimensional grid for the specified set of points.
  template<std::ranges::viewable_range Points>
    requires impl::can_grid<Points&&, real_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return Grid{std::forward<Points>(points), spacing_};
  }

private:

  real_t spacing_;

}; // class GridFactory

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
