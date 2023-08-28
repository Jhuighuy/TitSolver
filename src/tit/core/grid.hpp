/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include "tit/core/assert.hpp"
#include "tit/core/bbox.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/math.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"

#include <cmath>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** K-dimensional grid.
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
  using PointBBox = decltype(BBox(std::declval<Point>()));
  /** Numeric type used by the point type. */
  using Real = vec_num_t<Point>;
  /** Numeric type used by the point type. */
  static constexpr auto Dim = vec_dim_v<Point>;

private:

  Points _points;
  PointBBox _grid_bbox;
  Vec<size_t, Dim> _num_cells;
  Point _cell_size;
  Multivector<size_t> _cell_points;

public:

  constexpr explicit Grid(Points points, Real spacing = 2 * 0.6 / 80.0)
      : _points{std::move(points)} {
    TIT_ASSERT(spacing > 0, "Spacing must be positive.");
    if (std::ranges::empty(_points)) return;
    // Build the grid.
    _build_grid(spacing);
  }

private:

  // Compute index of the point.
  constexpr auto _point_to_cell_index(Point point) const noexcept -> size_t {
    point -= _grid_bbox.low, point /= _cell_size;
    auto index = static_cast<size_t>(std::floor(point[0]));
    for (size_t i = 1; i < Dim; ++i) {
      index *= _num_cells[i];
      index += static_cast<size_t>(std::floor(point[i]));
    }
    return index;
  }
  constexpr auto _point_to_cell_md_index(Point point) const noexcept {
    point -= _grid_bbox.low, point /= _cell_size;
    Vec<size_t, Dim> md_index;
    for (size_t i = 0; i < Dim; ++i) {
      md_index[i] = static_cast<size_t>(std::floor(point[i]));
    }
    return md_index;
  }

  constexpr void _build_grid(Real spacing) {
    // Compute grid bounding box.
    _grid_bbox = BBox{_points[0]};
    for (const auto& p : _points | std::views::drop(1)) _grid_bbox.update(p);
    _grid_bbox.low -= 0.5 * spacing, _grid_bbox.high += 0.5 * spacing;
    // Compute number of cells and cell sizes.
    const auto extents = _grid_bbox.extents();
    const auto approx_num_cells = extents / spacing;
    // TODO: refactor these by introducing functionality into `Vec`.
    auto total_num_cells = size_t{1};
    for (size_t i = 0; i < Dim; ++i) {
      _num_cells[i] = static_cast<size_t>(std::ceil(approx_num_cells[i]));
      total_num_cells *= _num_cells[i];
    }
    _cell_size = extents / _num_cells;
    // Pack the points into a multivector.
    _cell_points.assemble_tall(
        total_num_cells,
#if TIT_IWYU
        // IWYU's clang has no `std::views::enumerate`.
        std::views::single(std::tuple{size_t{0}, _points[0]}),
#else
        std::views::enumerate(_points),
#endif
        [&](auto index_and_point) {
          const auto& [_, point] = index_and_point;
          return _point_to_cell_index(point);
        },
        [](auto index_and_point) {
          const auto& [point_index, _] = index_and_point;
          return point_index;
        });
  }

public:

  /** Find the points within the radius to the given point. */
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Point search_point, Real search_radius,
                        OutIter out) const noexcept -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    const auto search_dist = pow2(search_radius);
    // Convert point to bounding box.
    const auto search_bbox =
        BBox{search_point - Point(search_radius) - 0.5 * _cell_size,
             search_point + Point(search_radius) + 0.5 * _cell_size};
    const auto low = _point_to_cell_md_index( //
        _grid_bbox.clamp(search_bbox.low) + 0.5 * _cell_size);
    const auto high = _point_to_cell_md_index(
        _grid_bbox.clamp(search_bbox.high) - 0.5 * _cell_size);
    for (size_t i = low[0]; i <= high[0]; ++i) {
      for (size_t j = low[1]; j <= high[1]; ++j) {
        const size_t cell_index = i * _num_cells[1] + j;
        for (size_t k : _cell_points[cell_index]) {
          const auto dist = norm2(search_point - _points[k]);
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
concept _can_grid = requires { Grid{std::declval<Args>()...}; };

/******************************************************************************\
 ** K-dimensional tree factory.
\******************************************************************************/
class GridFactory final {
public:

  /** Construct a K-dimensional tree factory. */
  constexpr GridFactory() {}

  /** Produce a K-dimensional tree for the specified set op points. */
  template<std::ranges::viewable_range Points>
    requires _can_grid<Points>
  constexpr auto operator()(Points&& points) const noexcept {
    return Grid{std::forward<Points>(points)};
  }

}; // class GridFactory

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
