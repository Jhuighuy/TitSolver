/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cmath>
#include <iterator> // IWYU pragma: keep
#include <ranges>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math_utils.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"

namespace tit::geom {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Uniform multidimensional grid.
\******************************************************************************/
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class Grid final {
public:

  /** Point type. */
  using Point = std::ranges::range_value_t<Points>;
  /** Bounding box type. */
  using PointBBox = bbox_t<Point>;

  /** Numeric type used by the point type. */
  using Real = vec_num_t<Point>;
  /** Numeric type used by the point type. */
  static constexpr auto Dim = vec_dim_v<Point>;

private:

  Points points_;
  PointBBox grid_bbox_;
  Vec<size_t, Dim> num_cells_;
  Point cell_size_;
  Multivector<size_t> cell_points_;

public:

  /** Initialize and build the multidimensional grid.
   ** @param spacing Grid cell size, typically 2x of the particle spacing. */
  constexpr explicit Grid(Points points, Real spacing)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("tit::Grid::Grid()");
    TIT_ASSERT(spacing > 0.0, "Spacing must be positive.");
    if (std::ranges::empty(points_)) return;
    // Build the grid.
    _build_grid(spacing);
  }

private:

  // Compute index of the point.
  constexpr auto _point_to_cell_index(Point point) const noexcept -> size_t {
    point -= grid_bbox_.low(), point /= cell_size_;
    auto index = static_cast<size_t>(std::floor(point[0]));
    for (size_t i = 1; i < Dim; ++i) {
      index *= num_cells_[i];
      index += static_cast<size_t>(std::floor(point[i]));
    }
    return index;
  }
  constexpr auto _point_to_cell_md_index(Point point) const noexcept {
    point -= grid_bbox_.low(), point /= cell_size_;
    Vec<size_t, Dim> md_index;
    for (size_t i = 0; i < Dim; ++i) {
      md_index[i] = static_cast<size_t>(std::floor(point[i]));
    }
    return md_index;
  }

  constexpr void _build_grid(Real spacing) {
    // Compute grid bounding box.
    grid_bbox_ = BBox{points_[0]};
    for (auto const& p : points_ | std::views::drop(1)) grid_bbox_.update(p);
    grid_bbox_.extend(0.5 * Point(spacing));
    // Compute number of cells and cell sizes.
    auto const extents = grid_bbox_.extents();
    auto const approx_num_cells = extents / spacing;
    // TODO: refactor these by introducing functionality into `Vec`.
    auto total_num_cells = 1UZ;
    for (size_t i = 0; i < Dim; ++i) {
      num_cells_[i] = static_cast<size_t>(std::ceil(approx_num_cells[i]));
      total_num_cells *= num_cells_[i];
    }
    cell_size_ = extents / num_cells_;
    // Pack the points into a multivector.
    cell_points_.assemble_tall(
        total_num_cells, std::views::enumerate(points_),
        [this](auto index_and_point) {
          auto const& [_, point] = index_and_point;
          return _point_to_cell_index(point);
        },
        [](auto index_and_point) {
          auto const& [point_index, _] = index_and_point;
          return point_index;
        });
  }

public:

  /** Find the points within the radius to the given point. */
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Point search_point, Real search_radius,
                        OutIter out) const noexcept -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    auto const search_dist = pow2(search_radius);
    // Convert point to bounding box.
    auto const search_bbox =
        BBox{search_point - Point(search_radius) - 0.5 * cell_size_,
             search_point + Point(search_radius) + 0.5 * cell_size_};
    auto const low = _point_to_cell_md_index( //
        grid_bbox_.clamp(search_bbox.low()) + 0.5 * cell_size_);
    auto const high = _point_to_cell_md_index(
        grid_bbox_.clamp(search_bbox.high()) - 0.5 * cell_size_);
    for (size_t i = low[0]; i <= high[0]; ++i) {
      for (size_t j = low[1]; j <= high[1]; ++j) {
        size_t const cell_index = i * num_cells_[1] + j;
        for (size_t const k : cell_points_[cell_index]) {
          auto const dist = norm2(search_point - points_[k]);
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

template<class... Args>
concept can_grid_ = requires { Grid{std::declval<Args>()...}; };

/******************************************************************************\
 ** Multidimensional grid factory.
\******************************************************************************/
class GridFactory final {
private:

  real_t spacing_;

public:

  /** Construct a multidimensional grid factory.
   ** @param spacing Grid cell size, typically 2x of the particle spacing. */
  constexpr explicit GridFactory(real_t spacing) : spacing_{spacing} {
    TIT_ASSERT(spacing_ > 0.0, "Spacing must be positive.");
  }

  /** Produce a multidimensional grid for the specified set of points. */
  template<std::ranges::viewable_range Points>
    requires can_grid_<Points, real_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return Grid{std::forward<Points>(points), spacing_};
  }

}; // class GridFactory

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::geom
