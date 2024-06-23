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

  /// Numeric type used by the point type.
  using Real = vec_num_t<Vec>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<Vec>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize and build the multidimensional grid.
  ///
  /// @param spacing Grid cell size, typically 2x of the particle spacing.
  constexpr explicit Grid(Points points, Real spacing)
      : points_{std::move(points)} {
    TIT_PROFILE_SECTION("tit::Grid::Grid()");
    TIT_ASSERT(spacing > 0.0, "Spacing must be positive.");
    if (std::ranges::empty(points_)) return;
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

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Vec search_point,
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
    if constexpr (Dim == 2) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          const size_t cell_index = i * num_cells_[1] + j;
          for (const size_t k : cell_points_[cell_index]) {
            const auto dist = norm2(search_point - points_[k]);
            if (dist < search_dist) *out++ = k;
          }
        }
      }
    } else if constexpr (Dim == 3) {
      for (size_t i = low[0]; i <= high[0]; ++i) {
        for (size_t j = low[1]; j <= high[1]; ++j) {
          for (size_t k = low[2]; k <= high[2]; ++k) {
            const size_t cell_index =
                i * num_cells_[1] * num_cells_[2] + j * num_cells_[2] + k;
            for (const size_t l : cell_points_[cell_index]) {
              const auto dist = norm2(search_point - points_[l]);
              if (dist < search_dist) *out++ = l;
            }
          }
        }
      }
    }
    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Compute index of the point.
  constexpr auto _point_to_cell_mdindex(Vec point) const noexcept {
    point = (point - grid_bbox_.low()) / cell_size_;
    return static_vec_cast<size_t>(point);
  }

  constexpr auto _point_to_cell_index(Vec point) const noexcept -> size_t {
    const auto mdindex = _point_to_cell_mdindex(point);
    auto index = mdindex[0];
    for (size_t i = 1; i < Dim; ++i) {
      index = num_cells_[i] * index + mdindex[i];
    }
    return index;
  }

  Points points_;
  BBox<Vec> grid_bbox_;
  tit::Vec<size_t, Dim> num_cells_;
  Vec cell_size_;
  Multivector<size_t> cell_points_;

}; // class Grid

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
Grid(Points&&, Args...) -> Grid<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    requires deduce_constructible_from<Grid, Points&&, real_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return Grid{std::forward<Points>(points), spacing_};
  }

private:

  real_t spacing_;

}; // class GridFactory

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
